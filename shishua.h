#ifndef SHISHUA_H
#define SHISHUA_H
#include <immintrin.h>
typedef struct prng_state {
  __m256i state[3];
  __m256i counter;
} prng_state;

// buf's size must be a multiple of 32 bytes.
inline void prng_gen(prng_state *s, __uint64_t buf[], __uint64_t size) {
  __m256i s0 = s->state[0], s1 = s->state[1], s2 = s->state[2], counter = s->counter;
  __m256i t1, u1, t2, u2,
     // The following shuffles move weak (low-diffusion) 32-bit parts of 64-bit
     // additions to strong positions for enrichment. The low 32-bit part of a
     // 64-bit chunk never moves to the same 64-bit chunk as its high part. They
     // do not remain in the same chunk. Each part eventually reaches all
     // positions ringwise: A to B, B to C, …, H to A.
     // You may notice that they are simply 256-bit rotations (96 and 160).
     shuf1 = _mm256_set_epi32(4, 3, 2, 1, 0, 7, 6, 5),
     shuf2 = _mm256_set_epi32(2, 1, 0, 7, 6, 5, 4, 3),
     increment = _mm256_set_epi64x(1, 3, 5, 7);
  for (__uint64_t i = 0; i < size; i += 4) {
    // The counter is not necessary to beat PractRand.
    // It sets a lower bound of 2^71 bytes to the period,
    // or about 7 millenia at 10 GiB/s.
    // The increments are picked as odd numbers,
    // since only coprimes of the base cover the full cycle,
    // and all odd numbers are coprime of 2.
    // I use different odd numbers for each 64-bit chunk
    // for a tiny amount of variation stirring.
    // I used the smallest odd numbers to avoid having a magic number.
    //_mm256_store_si256((__m256i*)&b[i], s0);
    buf[i+0] = _mm256_extract_epi64(s0, 0);
    buf[i+1] = _mm256_extract_epi64(s0, 1);
    buf[i+2] = _mm256_extract_epi64(s0, 2);
    buf[i+3] = _mm256_extract_epi64(s0, 3);
    if (i % 16 == 0) {
      // I apply the counter to s2,
      // since it is the one whose shift loses most entropy.
      s2 = _mm256_add_epi64(s2, counter);
      counter = _mm256_add_epi64(counter, increment);
    }
    // SIMD does not support rotations. Shift is the next best thing to entangle
    // bits with other 32-bit positions. We must shift by an odd number so that
    // each bit reaches all 64-bit positions, not just half. We must lose bits
    // of information, so we minimize it: 1 and 3. We use different shift values
    // to increase divergence between the two sides. We use rightward shift
    // because the rightmost bits have the least diffusion in addition (the low
    // bit is just a XOR of the low bits).
    u1 = _mm256_srli_epi64(s1, 1);               u2 = _mm256_srli_epi64(s2, 3);
    t1 = _mm256_permutevar8x32_epi32(s1, shuf1); t2 = _mm256_permutevar8x32_epi32(s2, shuf2);
    // Addition is the main source of diffusion. Storing the output in the state
    // keeps that diffusion permanently.
    s1 = _mm256_add_epi64(t1, u1);               s2 = _mm256_add_epi64(t2, u2);

    // Two orthogonally grown pieces evolving independently.
    s0 = _mm256_xor_si256(u1, t2);
  }
  s->state[0] = s0; s->state[1] = s1; s->state[2] = s2; s->counter = counter;
}

// Nothing up my sleeve: those are the hex digits of Φ,
// the least approximable irrational number.
// $ echo 'scale=310;obase=16;(sqrt(5)-1)/2' | bc
static __uint64_t phi[8] = {
  0x9E3779B97F4A7C15, 0xF39CC0605CEDC834, 0x1082276BF3A27251, 0xF86C6A11D0C18E95,
  0x2767F0B153D27B7F, 0x0347045B5BF1827F, 0x01886F0928403002, 0xC1D64BA40F335E36,
};

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  s.counter = _mm256_set_epi64x(0, 0, 0, 0);
# define ROUNDS 8
  __uint64_t buf[4 * ROUNDS];  // 4 64-bit numbers per 256-bit SIMD.
  // Diffuse first two seed elements in s0, then the lat two. Same for s1.
  // We must keep half of the state unchanged so users cannot set a bad state.
# define INIT(A, B) \
  s.state[1] = _mm256_set_epi64x(phi[3], phi[2] ^ seed[A + 1], phi[1], phi[0] ^ seed[A]); \
  s.state[2] = _mm256_set_epi64x(phi[7], phi[6] ^ seed[B + 1], phi[5], phi[4] ^ seed[B]); \
  prng_gen(&s, buf, 4 * ROUNDS);
  INIT(0, 2);
  INIT(2, 0);
# undef INIT
# undef ROUNDS
  return s;
}
#endif
