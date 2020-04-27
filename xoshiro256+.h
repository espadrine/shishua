#ifndef XOSHIRO_H
#define XOSHIRO_H

#include <stdint.h>
#include <stddef.h>

// Eight alternating Xoshiro256+ states benefitting from SIMD.
// Code from: http://prng.di.unimi.it/xoshiro256plus.c
// Speed comparison: http://prng.di.unimi.it/#speed
// where it is presented as the very fastest of the whole benchmark.
// Note: it fails PractRand BRank at 512 MiB.
// It mentions that the lowest three bits fail linearity tests,
// but it claims to be faster that way.
// I kept it because it lets us compare the SIMD version (which is the fastest).

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))

typedef struct prng_state {
  uint64_t state[4];
} prng_state;

// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint64_t buf[], size_t size) {
  uint64_t t;
  size_t n = size / 8;
  uint64_t *b = (uint64_t *)buf;
  for (size_t i = 0; i < n; i++) {
    b[i] = s->state[0] + s->state[3];

    t = s->state[1] << 17;

    s->state[2] ^= s->state[0];
    s->state[3] ^= s->state[1];
    s->state[1] ^= s->state[2];
    s->state[0] ^= s->state[3];

    s->state[2] ^= t;

    s->state[3] = ROTL(s->state[3], 45);
  }
}

// The original code has this to say:
//
// > The state must be seeded so that it is not everywhere zero. If you have
// > a 64-bit seed, we suggest to seed a splitmix64 generator and use its
// > output to fill s.
//
// We force to have at least one bit set.
// Since SHISHUA can handle any seed, including the zero seed and the seed that
// minimizes the amounts of bits set in the state after intialization, it seems
// fair. Ignoring bad splitmix64 gammas would hide severe seeding faults.
prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  for (size_t j = 0; j < 4; j++) { s.state[j] = seed[j]; }
  if (s.state[0] == 0) { s.state[0] = 1; }
  return s;
}
#endif
