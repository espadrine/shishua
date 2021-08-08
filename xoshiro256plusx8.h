#ifndef XOSHIRO_H
#define XOSHIRO_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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
  uint64_t t[XOSHIRO256_UNROLL];
  size_t n = size;
  uint64_t *b = (uint64_t *)buf;
  for (size_t i = 0; i < n; i += XOSHIRO256_UNROLL * 8) {
    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { prng_write_le64(&buf[i + (8 * j)], s->state[0][j] + s->state[3][j]); }

    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { t[j] = s->state[1][j] << 17; }

    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[2][j] ^= s->state[0][j]; }
    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[3][j] ^= s->state[1][j]; }
    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[1][j] ^= s->state[2][j]; }
    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[0][j] ^= s->state[3][j]; }

    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[2][j] ^= t[j]; }

    for (size_t j = 0; j < XOSHIRO256_UNROLL; j++) { s->state[3][j] = ROTL(s->state[3][j], 45); }
  }
}

void prng_init(prng_state *s, uint64_t seed[4]) {
  for (size_t i = 0; i < XOSHIRO256_UNROLL; i++) {
    for (size_t j = 0; j < 4; j++) { s->state[j][i] = seed[j] ^ (1 << i); }
  }
}
#endif
