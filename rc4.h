#ifndef RC4_H
#define RC4_H

#include <stdint.h>
#include <stddef.h>
// RC4 (aka arc4random).
// DO NOT USE THIS CODE FOR CRYPTOGRAPHIC PURPOSES.

typedef struct prng_state {
  uint8_t shuffle[256];
  uint8_t i, j;
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))
#define SWAP(i, j) { \
  uint8_t si = shuffle[(i)]; \
  shuffle[(i)] = shuffle[(j)]; \
  shuffle[(j)] = si; }

// buf's size must be a multiple of 8 bytes; it could get bytes one at a time.
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  uint8_t *shuffle = s->shuffle, *b = buf;
  for (size_t i = 0; i < size; i++) {
    s->i++; s->j += shuffle[s->i];
    SWAP(s->i, s->j);
    b[i] = shuffle[shuffle[s->i] + shuffle[s->j]];
  }
}

void prng_init(prng_state *s, uint64_t seed[4]) {
  uint8_t *shuffle = s->shuffle, *key = (uint8_t *)seed;
  for (size_t i = 0; i < 256; i++) { shuffle[i] = i; }
  for (size_t i = 0, j = 0; i < 256; i++, j = (j + shuffle[i] + key[i % 32]) % 256) {
    SWAP(i, j);
  }
  s->i = s->j = 0;
}
#endif
