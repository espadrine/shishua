#ifndef SHISHUA_H
#define SHISHUA_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <immintrin.h>
#include <assert.h>
typedef struct prng_state {
  __m256i state[2];
  __m256i output;
  __m256i counter;
} prng_state;

// buf's size must be a multiple of 32 bytes.
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  __m256i s0 = s->state[0], counter = s->counter,
          s1 = s->state[1],       o = s->output,
          t0, t1, t2, t3, u0, u1, u2, u3;
  // The following shuffles move weak (low-diffusion) 32-bit parts of 64-bit
  // additions to strong positions for enrichment. The low 32-bit part of a
  // 64-bit chunk never moves to the same 64-bit chunk as its high part.
  // They do not remain in the same chunk. Each part eventually reaches all
  // positions ringwise: A to B, B to C, …, H to A.
  // You may notice that they are simply 256-bit rotations (96 and 160).
  __m256i shu0 = _mm256_set_epi32(4, 3, 2, 1, 0, 7, 6, 5),
          shu1 = _mm256_set_epi32(2, 1, 0, 7, 6, 5, 4, 3);
  // The counter is not necessary to beat PractRand.
  // It sets a lower bound of 2^69 bytes = 512 EiB to the period,
  // or about 1 millenia at 10 GiB/s.
  // The increments are picked as odd numbers,
  // since only coprimes of the base cover the full cycle,
  // and all odd numbers are coprime of 2.
  // I use different odd numbers for each 64-bit chunk
  // for a tiny amount of variation stirring.
  // I used the smallest odd numbers to avoid having a magic number.
  __m256i increment = _mm256_set_epi64x(1, 3, 5, 7);

  // TODO: consider adding proper uneven write handling
  assert((size % 32 == 0) && "buf's size must be a multiple of 32 bytes.");

  for (size_t i = 0; i < size; i += 32) {
    if (buf != NULL) {
      _mm256_storeu_si256((__m256i*)&buf[i], o);
    }

    // I apply the counter to s1,
    // since it is the one whose shift loses most entropy.
    s1 = _mm256_add_epi64(s1, counter);
    counter = _mm256_add_epi64(counter, increment);

    // SIMD does not support rotations. Shift is the next best thing to entangle
    // bits with other 64-bit positions. We must shift by an odd number so that
    // each bit reaches all 64-bit positions, not just half. We must lose bits
    // of information, so we minimize it: 1 and 3. We use different shift values
    // to increase divergence between the two sides. We use rightward shift
    // because the rightmost bits have the least diffusion in addition (the low
    // bit is just a XOR of the low bits).
    u0 = _mm256_srli_epi64(s0, 1);              u1 = _mm256_srli_epi64(s1, 3);
    t0 = _mm256_permutevar8x32_epi32(s0, shu0); t1 = _mm256_permutevar8x32_epi32(s1, shu1);
    // Addition is the main source of diffusion.
    // Storing the output in the state keeps that diffusion permanently.
    s0 = _mm256_add_epi64(t0, u0);              s1 = _mm256_add_epi64(t1, u1);

    // Two orthogonally grown pieces evolving independently, XORed.
    o = _mm256_xor_si256(u0, t1);
  }
  s->state[0] = s0; s->counter = counter;
  s->state[1] = s1; s->output  = o;
}

// Nothing up my sleeve: those are the hex digits of Φ,
// the least approximable irrational number.
// $ echo 'scale=310;obase=16;(sqrt(5)-1)/2' | bc
static uint64_t phi[8] = {
  0x9E3779B97F4A7C15, 0xF39CC0605CEDC834, 0x1082276BF3A27251, 0xF86C6A11D0C18E95,
  0x2767F0B153D27B7F, 0x0347045B5BF1827F, 0x01886F0928403002, 0xC1D64BA40F335E36,
};

void prng_init(prng_state *s, uint64_t seed[4]) {
  memset(s, 0, sizeof(prng_state));
# define STEPS 5
# define ROUNDS 4
  uint8_t buf[32 * STEPS];  // 4 64-bit numbers per 256-bit SIMD.
  // Diffuse first two seed elements in s0, then the last two. Same for s1.
  // We must keep half of the state unchanged so users cannot set a bad state.
  s->state[0] = _mm256_set_epi64x(phi[3], phi[2] ^ seed[1], phi[1], phi[0] ^ seed[0]);
  s->state[1] = _mm256_set_epi64x(phi[7], phi[6] ^ seed[3], phi[5], phi[4] ^ seed[2]);
  for (size_t i = 0; i < ROUNDS; i++) {
    prng_gen(s, buf, 32 * STEPS);
    s->state[0] = s->state[1];
    s->state[1] = s->output;
  }
# undef STEPS
# undef ROUNDS
}
#endif
