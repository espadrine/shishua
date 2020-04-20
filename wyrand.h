#ifndef WYRAND_H
#define WYRAND_H

// wyrand: https://github.com/wangyi-fudan/wyhash/blob/master/wyhash.h
// A (hash-table) hash derivative.

typedef struct prng_state {
  __uint64_t counter;
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))

// buf's size must be a multiple of 8 bytes.
inline void prng_gen(prng_state *s, __uint64_t buf[], __uint64_t size) {
  for (__uint64_t i = 0; i < size; i++) {
    s->counter += 0xa0761d6478bd642full;
    __uint128_t r = (__uint128_t)(s->counter ^ 0xe7037ed1a0b428dbull) * s->counter;
    buf[i] = r ^ r >> 64;
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  s.counter = seed[0];
  return s;
}
#endif
