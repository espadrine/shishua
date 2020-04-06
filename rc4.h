#ifndef RC4_H
#define RC4_H

// RC4 (aka arc4random).
// DO NOT USE THIS CODE FOR CRYPTOGRAPHIC PURPOSES.

typedef struct prng_state {
  __uint8_t shuffle[256];
  __uint8_t i, j;
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))
#define SWAP(i, j) { \
  char si = shuffle[(i)]; \
  shuffle[(i)] = shuffle[(j)]; \
  shuffle[(j)] = si; }

// buf's size must be a multiple of 8 bytes; it could get bytes one at a time.
inline void prng_gen(prng_state *s, __uint64_t buf[], __uint64_t size) {
  __uint8_t *shuffle = s->shuffle, *b = buf;
  for (__uint64_t i = 0; i < size; i++) {
    for (char j = 0; j < 8; j++) {
      s->i++; s->j += shuffle[s->i];
      SWAP(s->i, s->j);
      b[8*i + j] = shuffle[shuffle[s->i] + shuffle[s->j]];
    }
  }
}

prng_state prng_init(SEEDTYPE seed[4]) {
  prng_state s;
  __uint8_t *shuffle = s.shuffle, *key = (char *)seed;
  for (int i = 0; i < 256; i++) { shuffle[i] = i; }
  for (int i = 0, j = 0; i < 256; i++, j = (j + shuffle[i] + key[i % 32]) % 256) {
    SWAP(i, j);
  }
  s.i = s.j = 0;
  return s;
}
#endif
