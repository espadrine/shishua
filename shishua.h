#ifndef SHISHUA_H
#define SHISHUA_H

#define SHISHUA_TARGET_SCALAR 0
#define SHISHUA_TARGET_AVX2 1
#define SHISHUA_TARGET_SSE2 2
#define SHISHUA_TARGET_NEON 3

#ifndef SHISHUA_TARGET
#  if defined(__AVX2__) && (defined(__x86_64__) || defined(_M_X64))
#    define SHISHUA_TARGET SHISHUA_TARGET_AVX2
#  elif defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#    define SHISHUA_TARGET SHISHUA_TARGET_SSE2
// GCC's NEON codegen leaves much to be desired, at least as of 9.2.0. The
// scalar path ends up being faster.
// Device: Google Pixel 2 XL, 2.46GHz Qualcomm Snapdragon 835
//   algorithm           |   GCC 9.2.0    |  Clang 9.0.1
//   shishua neon        | 0.2845 ns/byte | 0.0966 ns/byte
//   shishua scalar      | 0.2056 ns/byte | 0.2958 ns/byte
//   shishua half neon   | 0.5169 ns/byte | 0.1929 ns/byte
//   shishua half scalar | 0.2496 ns/byte | 0.2911 ns/byte
// Therefore, we only autoselect the NEON path on Clang, at least until GCC's
// NEON codegen improves.
#  elif (defined(__ARM_NEON) || defined(__ARM_NEON__)) && defined(__clang__)
#    define SHISHUA_TARGET SHISHUA_TARGET_NEON
#  else
#    define SHISHUA_TARGET SHISHUA_TARGET_SCALAR
#  endif
#endif

// These are all optional, with defining SHISHUA_TARGET_SCALAR, you only
// need this header.
#if SHISHUA_TARGET == SHISHUA_TARGET_AVX2
#  include "shishua-avx2.h"
#elif SHISHUA_TARGET == SHISHUA_TARGET_SSE2
#  include "shishua-sse2.h"
#elif SHISHUA_TARGET == SHISHUA_TARGET_NEON
#  include "shishua-neon.h"
#else // SHISHUA_TARGET == SHISHUA_TARGET_SCALAR

// Portable scalar implementation of shishua.
// Designed to balance performance and code size.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

// Note: While it is an array, a "lane" refers to 4 consecutive uint64_t.
typedef struct prng_state {
  uint64_t state[16];  // 4 lanes
  uint64_t output[16]; // 4 lanes, 2 parts
  uint64_t counter[4]; // 1 lane
} prng_state;

// buf could technically alias with prng_state, according to the compiler.
#if defined(__GNUC__) || defined(_MSC_VER)
#  define SHISHUA_RESTRICT __restrict
#else
#  define SHISHUA_RESTRICT
#endif

// Writes a 64-bit little endian integer to dst
static inline void prng_write_le64(void *dst, uint64_t val) {
  // Define to write in native endianness with memcpy
  // Also, use memcpy on known little endian setups.
#if defined(SHISHUA_NATIVE_ENDIAN) \
   || defined(_WIN32) \
   || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) \
   || defined(__LITTLE_ENDIAN__)
  memcpy(dst, &val, sizeof(uint64_t));
#else
  // Byteshift write.
  uint8_t *d = (uint8_t *)dst;
  for (size_t i = 0; i < 8; i++) {
    d[i] = (uint8_t)(val & 0xff);
    val >>= 8;
  }
#endif
}

// buf's size must be a multiple of 128 bytes.
static inline void prng_gen(prng_state *SHISHUA_RESTRICT state, uint8_t *SHISHUA_RESTRICT buf, size_t size) {
  uint8_t *b = buf;
  // TODO: consider adding proper uneven write handling
  assert((size % 128 == 0) && "buf's size must be a multiple of 128 bytes.");

  for (size_t i = 0; i < size; i += 128) {
    // Write the current output block to state if it is not NULL
    if (buf != NULL) {
      for (size_t j = 0; j < 16; j++) {
        prng_write_le64(b, state->output[j]); b += 8;
      }
    }
    // Similar to SSE, use fixed iteration loops to reduce code complexity
    // and allow the compiler more control over optimization.
    for (size_t j = 0; j < 2; j++) {
      // I don't want to type this 15 times.
      uint64_t *s = &state->state[j * 8];  // 2 lanes
      uint64_t *o = &state->output[j * 4]; // 1 lane
      uint64_t t[8]; // temp buffer

      // I apply the counter to s1,
      // since it is the one whose shift loses most entropy.
      for (size_t k = 0; k < 4; k++) {
        s[k + 4] += state->counter[k];
      }

      // The following shuffles move weak (low-diffusion) 32-bit parts of 64-bit
      // additions to strong positions for enrichment. The low 32-bit part of a
      // 64-bit chunk never moves to the same 64-bit chunk as its high part.
      // They do not remain in the same chunk. Each part eventually reaches all
      // positions ringwise: A to B, B to C, …, H to A.
      //
      // You may notice that they are simply 256-bit rotations (96 and 160):
      //
      //   t0 = (s0 <<  96) | (s0 >> (256 -  96));
      //   t1 = (s1 << 160) | (s1 >> (256 - 160));
      //
      // The easiest way to do this would be to cast s and t to uint32_t *
      // and operate on them that way.
      //
      //   uint32_t *t0_32 = (uint32_t *)t0, *t1_32 = (uint32_t *)t1;
      //   uint32_t *s0_32 = (uint32_t *)s0, *s1_32 = (uint32_t *)s1;
      //   for (size_t k = 0; k < 4; k++) {
      //     t0_32[k] = s0_32[(k + 5) % 8];
      //     t1_32[k] = s1_32[(k + 3) % 8];
      //   }
      //
      // This is pretty, but it violates strict aliasing and relies on little
      // endian data layout.
      //
      // A common workaround to strict aliasing would be to use memcpy:
      //
      //   // legal casts
      //   unsigned char *t8 = (unsigned char *)t;
      //   unsigned char *s8 = (unsigned char *)s;
      //   memcpy(&t8[0], &s8[20], 32 - 20);
      //   memcpy(&t8[32 - 20], &s8[0], 20);
      //
      // However, this still doesn't fix the endianness issue, and is very
      // ugly.
      //
      // The only known solution which doesn't rely on endianness is to
      // read two 64-bit integers and do a funnel shift.

      // Lookup table for the _offsets_ in the shuffle. Even lanes rotate
      // by 5, odd lanes rotate by 3.
      // If it were by 32-bit lanes, it would be
      // { 5,6,7,0,1,2,3,4, 11,12,13,14,15,8,9,10 }
      const uint8_t shuf_offsets[16] = { 2,3,0,1, 5,6,7,4,   // left
                                         3,0,1,2, 6,7,4,5 }; // right
      for (size_t k = 0; k < 8; k++) {
        t[k] = (s[shuf_offsets[k]] >> 32) | (s[shuf_offsets[k + 8]] << 32);
      }

      for (size_t k = 0; k < 4; k++) {
        // SIMD does not support rotations. Shift is the next best thing to entangle
        // bits with other 64-bit positions. We must shift by an odd number so that
        // each bit reaches all 64-bit positions, not just half. We must lose bits
        // of information, so we minimize it: 1 and 3. We use different shift values
        // to increase divergence between the two sides. We use rightward shift
        // because the rightmost bits have the least diffusion in addition (the low
        // bit is just a XOR of the low bits).
        uint64_t u_lo = s[k + 0] >> 1;
        uint64_t u_hi = s[k + 4] >> 3;

        // Addition is the main source of diffusion.
        // Storing the output in the state keeps that diffusion permanently.
        s[k + 0] = u_lo + t[k + 0];
        s[k + 4] = u_hi + t[k + 4];

        // The first orthogonally grown piece evolving independently, XORed.
        o[k] = u_lo ^ t[k + 4];
      }
    }

    // Merge together.
    for (size_t j = 0; j < 4; j++) {
      // The second orthogonally grown piece evolving independently, XORed.
      state->output[j +  8] = state->state[j + 0] ^ state->state[j + 12];
      state->output[j + 12] = state->state[j + 8] ^ state->state[j +  4];
      // The counter is not necessary to beat PractRand.
      // It sets a lower bound of 2^71 bytes = 2 ZiB to the period,
      // or about 7 millenia at 10 GiB/s.
      // The increments are picked as odd numbers,
      // since only coprimes of the base cover the full cycle,
      // and all odd numbers are coprime of 2.
      // I use different odd numbers for each 64-bit chunk
      // for a tiny amount of variation stirring.
      // I used the smallest odd numbers to avoid having a magic number.
      //
      // For the scalar version, we calculate this dynamically, as it is
      // simple enough.
      state->counter[j] += 7 - (j * 2); // 7, 5, 3, 1
    }
  }
}
#undef SHISHUA_RESTRICT

// Nothing up my sleeve: those are the hex digits of Φ,
// the least approximable irrational number.
// $ echo 'scale=310;obase=16;(sqrt(5)-1)/2' | bc
static uint64_t phi[16] = {
  0x9E3779B97F4A7C15, 0xF39CC0605CEDC834, 0x1082276BF3A27251, 0xF86C6A11D0C18E95,
  0x2767F0B153D27B7F, 0x0347045B5BF1827F, 0x01886F0928403002, 0xC1D64BA40F335E36,
  0xF06AD7AE9717877E, 0x85839D6EFFBD7DC6, 0x64D325D1C5371682, 0xCADD0CCCFDFFBBE1,
  0x626E33B8D04B4331, 0xBBF73C790D94F79D, 0x471C4AB3ED3D82A5, 0xFEC507705E4AE6E5,
};

void prng_init(prng_state *s, uint64_t seed[4]) {
  memset(s, 0, sizeof(prng_state));
# define STEPS 1
# define ROUNDS 13
  // Diffuse first two seed elements in s0, then the last two. Same for s1.
  // We must keep half of the state unchanged so users cannot set a bad state.
  memcpy(s->state, phi, sizeof(phi));
  for (size_t i = 0; i < 4; i++) {
    s->state[i * 2 + 0] ^= seed[i];           // { s0,0,s1,0,s2,0,s3,0 }
    s->state[i * 2 + 8] ^= seed[(i + 2) % 4]; // { s2,0,s3,0,s0,0,s1,0 }
  }
  for (size_t i = 0; i < ROUNDS; i++) {
    prng_gen(s, NULL, 128 * STEPS);
    for (size_t j = 0; j < 4; j++) {
       s->state[j+ 0] = s->output[j+12];
       s->state[j+ 4] = s->output[j+ 8];
       s->state[j+ 8] = s->output[j+ 4];
       s->state[j+12] = s->output[j+ 0];
    }
  }
# undef STEPS
# undef ROUNDS
}
#endif // SHISHUA_TARGET == SHISHUA_TARGET_SCALAR
#endif // SHISHUA_SCALAR_H
