#ifndef WYRAND_H
#define WYRAND_H

#include <stdint.h>
#include <stddef.h>

// wyrand: https://github.com/wangyi-fudan/wyhash/blob/master/wyhash.h
// A (hash-table) hash derivative.

typedef struct prng_state {
  uint64_t counter;
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))
#ifdef __SIZEOF_INT128__
static inline uint64_t prng_mult128_xorfold(uint64_t lhs, uint64_t rhs) {
    __uint128_t product = (__uint128_t)lhs * rhs;
    return (uint64_t)(product ^ (product >> 64));
}
#elif defined(_M_X64) && defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_umul128)
static inline uint64_t prng_mult128_xorfold(uint64_t lhs, uint64_t rhs) {
    uint64_t hi;
    uint64_t lo = _umul128(lhs, rhs, &hi);
    return hi ^ lo;
}
#else
static inline uint64_t prng_mult128_xorfold(uint64_t lhs, uint64_t rhs) {
#define PRNG_MULL(x, y) ((uint64_t)(uint32_t)(x) * (uint64_t)(uint32_t)(y))
    uint64_t lo_lo = PRNG_MULL(lhs & 0xffffffff, rhs & 0xffffffff);
    uint64_t hi_lo = PRNG_MULL(lhs >> 32,        rhs & 0xffffffff);
    uint64_t lo_hi = PRNG_MULL(lhs & 0xffffffff, rhs >> 32);
    uint64_t hi_hi = PRNG_MULL(lhs >> 32,        rhs >> 32);
#undef PRNG_MULL
    uint64_t cross = hi_lo + (lo_lo >> 32) + (lo_hi & 0xffffffff);
    uint64_t lower = (cross << 32) | (lo_lo & 0xffffffff);
    uint64_t upper = hi_hi + (cross >> 32) + (lo_hi >> 32);
    return lower ^ upper;
}
#endif
// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint64_t buf[], size_t size) {
  for (size_t i = 0; i < size; i++) {
    s->counter += 0xa0761d6478bd642full;
    buf[i] = prng_mult128_xorfold(s->counter ^ 0xe7037ed1a0b428dbull, s->counter);
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  s.counter = seed[0];
  return s;
}
#endif
