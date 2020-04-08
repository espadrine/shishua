#ifndef WYRAND_H
#define WYRAND_H

// wyrand v6: https://github.com/wangyi-fudan/wyhash/blob/master/wyhash_v6.h
// A (hash-table) hash derivative.

typedef struct prng_state {
  __uint64_t counter;
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))

// buf's size must be a multiple of 8 bytes.
inline void prng_gen(prng_state *s, __uint64_t buf[], __uint64_t size) {
  for (__uint64_t i = 0; i < size; i++) {
    s->counter += 0xb10f1ea5b4358d87ull;
    __uint128_t r = (__uint128_t)(s->counter ^ 0x2e63952eb46a7127ull) * s->counter;
    buf[i] = r ^ r >> 64;
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  s.counter = seed[0];
  return s;
}
#endif
