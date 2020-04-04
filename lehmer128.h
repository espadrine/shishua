#ifndef LEHMER_H
#define LEHMER_H

// LEHMER128: https://lemire.me/blog/2019/03/19/the-fastest-conventional-random-number-generator-that-can-pass-big-crush/

typedef struct prng_state {
  __uint128_t state;
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))

// buf's size must be a multiple of 8 bytes.
inline void prng_gen(prng_state *s, __uint64_t buf[], __uint64_t size) {
  for (__uint64_t i = 0; i < size; i++) {
    buf[i] = (__uint128_t)(s->state *= 0xda942042e4dd58b5) >> 64;
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  s.state = seed[2];
  s.state <<= 64;
  s.state ^= seed[3];
  if (s.state == 0) { s.state = 1; }
  return s;
}
#endif
