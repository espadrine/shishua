// Test vectors for shishua
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifndef HEADER
#  define HEADER "shishua.h"
#endif
#include HEADER
#define phi phi_half
#define prng_state prng_state_half
#define prng_init prng_init_half
#define prng_gen prng_gen_half
#ifndef HEADER_HALF
#  define HEADER_HALF "shishua-half.h"
#endif
#include HEADER_HALF
#undef phi
#undef prng_state
#undef prng_init
#undef prng_gen

#include "test-vectors.h" // generated file

static size_t find_mismatch(const uint8_t *a, const uint8_t *b, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (a[i] != b[i])
      return i;
  }
  return size;
}

int main(void) {
  uint8_t buf[sizeof(shishua_vector_unseeded)] = {0};
  prng_state state;
  prng_init(&state, seed_zero);
  memset(buf, 0, sizeof(buf));
  prng_gen(&state, buf, sizeof(buf));

  size_t offset = find_mismatch(shishua_vector_unseeded, buf, sizeof(buf));
  if (offset >= sizeof(buf)) {
    printf("shishua unseeded: ok\n");
  } else {
    printf("shishua: mismatch at position %zu.\n", offset);
    printf("expected: %#02x, got %#02x\n", shishua_vector_unseeded[offset], buf[offset]);
    return 1;
  }
  prng_state_half state_half;
  prng_init_half(&state_half, seed_zero);
  memset(buf, 0, sizeof(buf));
  prng_gen_half(&state_half, buf, sizeof(buf));


  offset = find_mismatch(shishua_half_vector_unseeded, buf, sizeof(buf));
  if (offset >= sizeof(buf)) {
    printf("shishua half unseeded: ok\n");
  } else {
    printf("shishua half unseeded: mismatch at position %zu.\n", offset);
    printf("expected: %#02x, got %#02x\n", shishua_half_vector_unseeded[offset], buf[offset]);
    return 1;
  }
  prng_init(&state, seed_pi);
  memset(buf, 0, sizeof(buf));
  prng_gen(&state, buf, sizeof(buf));
  offset = find_mismatch(shishua_vector_seeded, buf, sizeof(buf));
  if (offset >= sizeof(buf)) {
    printf("shishua seeded: ok\n");
  } else {
    printf("shishua seeded: mismatch at position %zu.\n", offset);
    printf("expected: %#02x, got %#02x\n", shishua_vector_seeded[offset], buf[offset]);
    return 1;
  }
  prng_init_half(&state_half, seed_pi);
  memset(buf, 0, sizeof(buf));
  prng_gen_half(&state_half, buf, sizeof(buf));

  offset = find_mismatch(shishua_half_vector_seeded, buf, sizeof(buf));
  if (offset >= sizeof(buf)) {
    printf("shishua half seeded: ok\n");
  } else {
    printf("shishua half seeded: mismatch at position %zu.\n", offset);
    printf("expected: %#02x, got %#02x\n", shishua_half_vector_seeded[offset], buf[offset]);
    return 1;
  }
  return 0;
}

