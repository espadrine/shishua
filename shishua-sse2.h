// An SSE2/SSSE3 version of shishua. Slower than AVX2, but more compatible.
// Also compatible with 32-bit x86.
//
// SSSE3 is recommended, as it has the useful _mm_alignr_epi8 intrinsic.
// We can still emulate it on SSE2, but it is slower.
// Consider the half version if AVX2 is not available.
#ifndef SHISHUA_SSE2_H
#define SHISHUA_SSE2_H
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
// Note: cl.exe doesn't define __SSSE3__
#if defined(__SSSE3__) || defined(__AVX__)
#  include <tmmintrin.h> // SSSE3
#  define SHISHUA_ALIGNR_EPI8(hi, lo, amt) \
   _mm_alignr_epi8(hi, lo, amt)
#else
#  include <emmintrin.h> // SSE2
// Emulate _mm_alignr_epi8 for SSE2. It's a little slow.
// The compiler may convert it to a sequence of shufps instructions, which is
// perfectly fine.
#  define SHISHUA_ALIGNR_EPI8(hi, lo, amt) \
   _mm_or_si128( \
     _mm_slli_si128(hi, 16 - (amt)), \
     _mm_srli_si128(lo, amt) \
   )
#endif

typedef struct prng_state {
  __m128i state[8];
  __m128i output[8];
  __m128i counter[2];
} prng_state;

// Wrappers for x86 targets which usually lack these intrinsics.
// Don't call these with side effects.
#if defined(__x86_64__) || defined(_M_X64)
#   define SHISHUA_SET_EPI64X(b, a) _mm_set_epi64x(b, a)
#   define SHISHUA_CVTSI64_SI128(x) _mm_cvtsi64_si128(x)
#else
#   define SHISHUA_SET_EPI64X(b, a) \
      _mm_set_epi32( \
        (int)(((uint64_t)(b)) >> 32), \
        (int)(b), \
        (int)(((uint64_t)(a)) >> 32), \
        (int)(a) \
      )
#   define SHISHUA_CVTSI64_SI128(x) SHISHUA_SET_EPI64X(0, x)
#endif

// buf could technically alias with prng_state, according to the compiler.
#if defined(__GNUC__) || defined(_MSC_VER)
#  define SHISHUA_RESTRICT __restrict
#else
#  define SHISHUA_RESTRICT
#endif

// buf's size must be a multiple of 128 bytes.
static inline void prng_gen(prng_state *SHISHUA_RESTRICT s, uint8_t *SHISHUA_RESTRICT buf, size_t size) {
  __m128i counter_lo = s->counter[0], counter_hi = s->counter[1];
  // The counter is not necessary to beat PractRand.
  // It sets a lower bound of 2^71 bytes = 2 ZiB to the period,
  // or about 7 millenia at 10 GiB/s.
  // The increments are picked as odd numbers,
  // since only coprimes of the base cover the full cycle,
  // and all odd numbers are coprime of 2.
  // I use different odd numbers for each 64-bit chunk
  // for a tiny amount of variation stirring.
  // I used the smallest odd numbers to avoid having a magic number.

  // increment = { 7, 5, 3, 1 };
  __m128i increment_lo = SHISHUA_SET_EPI64X(5, 7);
  __m128i increment_hi = SHISHUA_SET_EPI64X(1, 3);

  // TODO: consider adding proper uneven write handling
  assert((size % 128 == 0) && "buf's size must be a multiple of 128 bytes.");

  for (size_t i = 0; i < size; i += 128) {
    // Write the current output block to state if it is not NULL
    if (buf != NULL) {
      for (size_t j = 0; j < 8; j++) {
        _mm_storeu_si128((__m128i *)&buf[i + (16 * j)], s->output[j]);
      }
    }

    // There are only 16 SSE registers (8 on i686), and we have to account for
    // temporary copies due to being stuck with 2-operand instructions.
    // Therefore, we use fixed iteration loops to reduce code complexity while
    // still allowing the compiler to easily unroll the loop.
    // We also try to keep variables active for as short as possible.
    for (size_t j = 0; j < 2; j++) {
      __m128i s_lo, s_hi, u0_lo, u0_hi, u1_lo, u1_hi, t_lo, t_hi;

      // Lane 0
      s_lo = s->state[4 * j + 0];
      s_hi = s->state[4 * j + 1];

      // SIMD does not support rotations. Shift is the next best thing to entangle
      // bits with other 64-bit positions. We must shift by an odd number so that
      // each bit reaches all 64-bit positions, not just half. We must lose bits
      // of information, so we minimize it: 1 and 3. We use different shift values
      // to increase divergence between the two sides. We use rightward shift
      // because the rightmost bits have the least diffusion in addition (the low
      // bit is just a XOR of the low bits).
      u0_lo = _mm_srli_epi64(s_lo, 1);
      u0_hi = _mm_srli_epi64(s_hi, 1);

      // The following shuffles move weak (low-diffusion) 32-bit parts of 64-bit
      // additions to strong positions for enrichment. The low 32-bit part of a
      // 64-bit chunk never moves to the same 64-bit chunk as its high part.
      // They do not remain in the same chunk. Each part eventually reaches all
      // positions ringwise: A to B, B to C, …, H to A.
      // You may notice that they are simply 256-bit rotations (96 and 160).
      // Note: This:
      //   x = (y << 96) | (y >> 160)
      // can be rewritten as this
      //   x_lo = (y_lo << 96) | (y_hi >> 32)
      //   x_hi = (y_hi << 96) | (y_lo >> 32)
      // which we can do with 2 _mm_alignr_epi8 instructions.
      t_lo = SHISHUA_ALIGNR_EPI8(s_lo, s_hi, 4);
      t_hi = SHISHUA_ALIGNR_EPI8(s_hi, s_lo, 4);

      // Addition is the main source of diffusion.
      // Storing the output in the state keeps that diffusion permanently.
      s->state[4 * j + 0] = _mm_add_epi64(t_lo, u0_lo);
      s->state[4 * j + 1] = _mm_add_epi64(t_hi, u0_hi);

      // Lane 1
      s_lo = s->state[4 * j + 2];
      s_hi = s->state[4 * j + 3];

      // I apply the counter to s1,
      // since it is the one whose shift loses most entropy.
      s_lo = _mm_add_epi64(s_lo, counter_lo);
      s_hi = _mm_add_epi64(s_hi, counter_hi);

      // Same as above but with different shifts
      u1_lo = _mm_srli_epi64(s_lo, 3);
      u1_hi = _mm_srli_epi64(s_hi, 3);

      t_lo = SHISHUA_ALIGNR_EPI8(s_hi, s_lo, 12);
      t_hi = SHISHUA_ALIGNR_EPI8(s_lo, s_hi, 12);

      s->state[4 * j + 2] = _mm_add_epi64(t_lo, u1_lo);
      s->state[4 * j + 3] = _mm_add_epi64(t_hi, u1_hi);

      // Merge lane 0 and lane 1
      // The first orthogonally grown piece evolving independently, XORed.
      s->output[2 * j + 0] = _mm_xor_si128(u0_lo, t_lo);
      s->output[2 * j + 1] = _mm_xor_si128(u0_hi, t_hi);
    }

    // The second orthogonally grown piece evolving independently, XORed.
    s->output[4] = _mm_xor_si128(s->state[0], s->state[6]);
    s->output[5] = _mm_xor_si128(s->state[1], s->state[7]);
    s->output[6] = _mm_xor_si128(s->state[4], s->state[2]);
    s->output[7] = _mm_xor_si128(s->state[5], s->state[3]);

    // Increment the counter
    counter_lo = _mm_add_epi64(counter_lo, increment_lo);
    counter_hi = _mm_add_epi64(counter_hi, increment_hi);
  }

  s->counter[0] = counter_lo;
  s->counter[1] = counter_hi;
}

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
  // Note: output is uninitialized at first, but since we pass NULL, its value
  // is initially ignored.
  s->counter[0] = _mm_setzero_si128();
  s->counter[1] = _mm_setzero_si128();
# define ROUNDS 13
# define STEPS 1
  // Diffuse first two seed elements in s0, then the last two. Same for s1.
  // We must keep half of the state unchanged so users cannot set a bad state.
  __m128i seed_0 = SHISHUA_CVTSI64_SI128(seed[0]);
  __m128i seed_1 = SHISHUA_CVTSI64_SI128(seed[1]);
  __m128i seed_2 = SHISHUA_CVTSI64_SI128(seed[2]);
  __m128i seed_3 = SHISHUA_CVTSI64_SI128(seed[3]);
  s->state[0] = _mm_xor_si128(seed_0, _mm_loadu_si128((__m128i *)&phi[ 0]));
  s->state[1] = _mm_xor_si128(seed_1, _mm_loadu_si128((__m128i *)&phi[ 2]));
  s->state[2] = _mm_xor_si128(seed_2, _mm_loadu_si128((__m128i *)&phi[ 4]));
  s->state[3] = _mm_xor_si128(seed_3, _mm_loadu_si128((__m128i *)&phi[ 6]));
  s->state[4] = _mm_xor_si128(seed_2, _mm_loadu_si128((__m128i *)&phi[ 8]));
  s->state[5] = _mm_xor_si128(seed_3, _mm_loadu_si128((__m128i *)&phi[10]));
  s->state[6] = _mm_xor_si128(seed_0, _mm_loadu_si128((__m128i *)&phi[12]));
  s->state[7] = _mm_xor_si128(seed_1, _mm_loadu_si128((__m128i *)&phi[14]));

  for (int i = 0; i < ROUNDS; i++) {
    prng_gen(s, NULL, 128 * STEPS);
    s->state[0] = s->output[6];  s->state[1] = s->output[7];
    s->state[2] = s->output[4];  s->state[3] = s->output[5];
    s->state[4] = s->output[2];  s->state[5] = s->output[3];
    s->state[6] = s->output[0];  s->state[7] = s->output[1];
  }
# undef STEPS
# undef ROUNDS
}
#undef SHISHUA_CVTSI64_SI128
#undef SHISHUA_ALIGNR_EPI8
#undef SHISHUA_SET_EPI64X
#undef SHISHUA_RESTRICT
#endif
