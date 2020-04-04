#ifndef ROMU_H
#define ROMU_H

typedef struct prng_state {
  __uint64_t state[3];
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))

// buf's size must be a multiple of 8 bytes.
inline void prng_gen(prng_state *s, __uint64_t buf[], __uint64_t size) {
  for (__uint64_t i = 0; i < size; i++) {
    // ROMU: http://www.romu-random.org/romupaper.pdf
    __uint64_t xp = s->state[0], yp = s->state[1], zp = s->state[2];
    s->state[0] = 15241094284759029579u * zp;
    s->state[1] = ROTL(yp - xp, 12);
    s->state[2] = ROTL(zp - yp, 44);
    buf[i] = xp;
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  for (char i = 0; i < 3; i++) { s.state[i] = seed[i]; }
  if (s.state[2] == 0) { s.state[2] = 1; }
  return s;
}
#endif
