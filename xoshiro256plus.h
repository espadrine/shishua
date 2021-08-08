#ifndef XOSHIRO_H
#define XOSHIRO_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

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

// Writes a 64-bit little endian integer to dst
static inline void prng_write_le64(void *dst, uint64_t val) {
  // Define to write in native endianness with memcpy
  // Also, use memcpy on known little endian setups.
# if defined(SHISHUA_NATIVE_ENDIAN) \
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

// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  uint64_t t;
  size_t n = size;
  uint8_t *b = (uint8_t *)buf;
  for (size_t i = 0; i < n; i += 8) {
    prng_write_le64(&b[i], s->state[0] + s->state[3]);

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
// minimizes the amounts of bits set in the state after initialization, it seems
// fair. Ignoring bad splitmix64 gammas would hide severe seeding faults.
void prng_init(prng_state *s, uint64_t seed[4]) {
  for (size_t j = 0; j < 4; j++) { s->state[j] = seed[j]; }
  if (s->state[0] == 0) { s->state[0] = 1; }
}
#endif
