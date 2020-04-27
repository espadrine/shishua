#ifndef LEHMER_H
#define LEHMER_H

#include <stdint.h>
#include <stddef.h>
// LEHMER128: https://lemire.me/blog/2019/03/19/the-fastest-conventional-random-number-generator-that-can-pass-big-crush/

#ifdef __SIZEOF_INT128__
typedef struct prng_state {
  __uint128_t state;
} prng_state;

// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  uint64_t *b = (uint64_t *)buf;
  size_t n = size / 8;
  for (size_t i = 0; i < n; i++) {
    b[i] = (__uint128_t)(s->state *= 0xda942042e4dd58b5) >> 64;
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  s.state = seed[0] ^ seed[2];
  s.state <<= 64;
  s.state ^= seed[1] ^ seed[3];
  if (s.state == 0) { s.state = 1; }
  return s;
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
static inline void prng_gen(prng_state *s, uint64_t buf[], size_t size) {
  for (size_t i = 0; i < size; i++) {
    prng_mult128by64(s->state, 0xda942042e4dd58b5);
    buf[i] = s->state[1];
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  s.state[1] = seed[0] ^ seed[2];
  s.state[0] = seed[1] ^ seed[3];
  if (s.state[0] == 0 && s.state[1] == 0) { s.state[0] = 1; }
  return s;
}
#endif // __SIZEOF_INT128__
#endif
