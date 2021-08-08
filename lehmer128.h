#ifndef LEHMER_H
#define LEHMER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// LEHMER128: https://lemire.me/blog/2019/03/19/the-fastest-conventional-random-number-generator-that-can-pass-big-crush/

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

#ifdef __SIZEOF_INT128__
typedef struct prng_state {
  __uint128_t state;
} prng_state;

// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  size_t n = size;
  for (size_t i = 0; i < n; i += 8) {
    prng_write_le64(&buf[i], (__uint128_t)(s->state *= 0xda942042e4dd58b5) >> 64);
  }
}

void prng_init(prng_state *s, uint64_t seed[4]) {
  s->state = seed[0] ^ seed[2];
  s->state <<= 64;
  s->state ^= seed[1] ^ seed[3];
  if (s->state == 0) { s->state = 1; }
}
#else

typedef struct prng_state {
  uint64_t state[2];
} prng_state;

#if defined(_M_X64) && defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_umul128)
static inline void prng_mult128by64(uint64_t lhs[2], uint64_t rhs) {
    uint64_t hi;
    uint64_t lo = _umul128(lhs[0], rhs, &hi);
    lhs[0] = lo;
    lhs[1] = hi + (lhs[1] * rhs);
}
#else
static inline void prng_mult128by64(uint64_t lhs[2], uint64_t rhs) {
#define PRNG_MULL(x, y) ((uint64_t)(uint32_t)(x) * (uint64_t)(uint32_t)(y))
    uint64_t lo_lo = PRNG_MULL(lhs[0] & 0xffffffff, rhs & 0xffffffff);
    uint64_t hi_lo = PRNG_MULL(lhs[0] >> 32,        rhs & 0xffffffff);
    uint64_t lo_hi = PRNG_MULL(lhs[0] & 0xffffffff, rhs >> 32);
    uint64_t hi_hi = PRNG_MULL(lhs[0] >> 32,        rhs >> 32);
#undef PRNG_MULL
    uint64_t cross = hi_lo + (lo_lo >> 32) + (lo_hi & 0xffffffff);
    uint64_t lower = (cross << 32) | (lo_lo & 0xffffffff);
    uint64_t upper = hi_hi + (cross >> 32) + (lo_hi >> 32);
    lhs[0] = lower;
    lhs[1] = upper + (lhs[1] * rhs);
}
#endif
// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  for (size_t i = 0; i < size; i += 8) {
    prng_mult128by64(s->state, 0xda942042e4dd58b5);
    prng_write_le64(&buf[i], s->state[1]);
  }
}

void prng_init(prng_state *s, uint64_t seed[4]) {
  s->state[1] = seed[0] ^ seed[2];
  s->state[0] = seed[1] ^ seed[3];
  if (s->state[0] == 0 && s->state[1] == 0) { s->state[0] = 1; }
}
#endif // __SIZEOF_INT128__
#endif
