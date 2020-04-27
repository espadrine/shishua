#ifndef XOSHIRO_H
#define XOSHIRO_H
#include <stdint.h>
#include <stddef.h>
// Eight alternating Xoshiro256+ states benefitting from SIMD.
// Code from: http://prng.di.unimi.it/xoshiro256+-vect-speed.c
// Speed comparison: http://prng.di.unimi.it/#speed
// where it is presented as the very fastest of the whole benchmark.
// Note: it fails PractRand BRank at 512 MiB,
// likely because of an interseed correlation and poor diffusion.
// It is sad, but I rely on the seeding procedure coded by Vigna.

#define XOSHIRO256_UNROLL (8)
#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))

typedef struct prng_state {
  uint64_t state[4][XOSHIRO256_UNROLL];
} prng_state;

// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint64_t buf[], size_t size) {
  uint64_t t[XOSHIRO256_UNROLL];
  size_t n = size / 8;
  uint64_t *b = (uint64_t *)buf;
  for (size_t i = 0; i < n; i += XOSHIRO256_UNROLL) {
    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { buf[i + j] = s->state[0][j] + s->state[3][j]; }

    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { t[j] = s->state[1][j] << 17; }

    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[2][j] ^= s->state[0][j]; }
    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[3][j] ^= s->state[1][j]; }
    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[1][j] ^= s->state[2][j]; }
    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[0][j] ^= s->state[3][j]; }

    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[2][j] ^= t[j]; }

    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[3][j] = ROTL(s->state[3][j], 45); }
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  for (size_t i = 0; i < XOSHIRO256_UNROLL; i++) {
    for (size_t j = 0; j < 4; j++) { s.state[j][i] = seed[j] ^ (1 << i); }
  }
  return s;
}
#endif
