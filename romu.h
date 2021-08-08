#ifndef ROMU_H
#define ROMU_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct prng_state {
  uint64_t state[3];
} prng_state;

#define ROTL(a,n) (((a) << (n)) | ((a) >> (64 - (n))))

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

// buf's size must be a multiple of 8 bytes.
static inline void prng_gen(prng_state *s, uint8_t buf[], size_t size) {
  size_t n = size;
  for (size_t i = 0; i < n; i += 8) {
    // ROMU: http://www.romu-random.org/romupaper.pdf
    uint64_t xp = s->state[0], yp = s->state[1], zp = s->state[2];
    s->state[0] = 15241094284759029579u * zp;
    s->state[1] = ROTL(yp - xp, 12);
    s->state[2] = ROTL(zp - yp, 44);
    prng_write_le64(&buf[i], xp);
  }
}

void prng_init(prng_state *s, uint64_t seed[4]) {
  for (size_t i = 0; i < 3; i++) { s->state[i] = seed[i]; }
  if (s->state[2] == 0) { s->state[2] = 1; }
}
#endif
