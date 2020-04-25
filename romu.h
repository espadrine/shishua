#ifndef ROMU_H
#define ROMU_H
#include <stdint.h>
#include <stddef.h>
typedef struct prng_state {
  uint64_t state[3];
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))

// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  size_t n = size / 8;
  uint64_t *b = (uint64_t *)buf;
  for (size_t i = 0; i < n; i++) {
    // ROMU: http://www.romu-random.org/romupaper.pdf
    uint64_t xp = s->state[0], yp = s->state[1], zp = s->state[2];
    s->state[0] = 15241094284759029579u * zp;
    s->state[1] = ROTL(yp - xp, 12);
    s->state[2] = ROTL(zp - yp, 44);
    b[i] = xp;
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  for (size_t i = 0; i < 3; i++) { s.state[i] = seed[i]; }
  if (s.state[2] == 0) { s.state[2] = 1; }
  return s;
}
#endif
