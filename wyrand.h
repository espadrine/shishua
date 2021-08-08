#ifndef WYRAND_H
#define WYRAND_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// wyrand: https://github.com/wangyi-fudan/wyhash/blob/master/wyhash.h
// A (hash-table) hash derivative.

typedef struct prng_state {
  uint64_t counter;
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
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  for (size_t i = 0; i < size; i += 8) {
    s->counter += 0xa0761d6478bd642full;
    prng_write_le64(&buf[i], prng_mult128_xorfold(s->counter ^ 0xe7037ed1a0b428dbull, s->counter));
  }
}

void prng_init(prng_state *s, uint64_t seed[4]) {
  s->counter = seed[0];
}
#endif
