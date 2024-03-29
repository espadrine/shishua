#include <stdio.h>
#include <inttypes.h>

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

static void print_buffer(FILE *f, const char *name, const uint8_t *data, size_t len) {
  fprintf(f, "static const uint8_t %s[%zu] = {", name, len);
  size_t columns = (80 - 2) / 6;
  for (size_t i = 0; i < len; i++) {
    if (i % columns == 0) {
      fprintf(f, "\n "); // line break
    }
    fprintf(f, " 0x%02x,", data[i]);
  }
  fprintf(f, "\n};\n");
}

static void print_seed(FILE *f, const char *name, const uint64_t *seed)
{
  fprintf(f, "static uint64_t %s[4] = {\n", name);
  for (size_t i = 0; i < 4; i++) {
    fprintf(f, "  0x%016"PRIx64",\n", seed[i]);
  }
  fprintf(f, "};\n");
}

int main() {

  uint8_t buf[512] = {0};
  uint64_t seed_zero[4] = {0};
  // Digits of pi in big endian
  uint64_t seed_pi[4] = { 0x243f6a8885a308d3, 0x13198a2e03707344,
                          0xa409382229f31d00, 0x82efa98ec4e6c894 };
  prng_state state;
  prng_init(&state, seed_zero);
  FILE *f = fopen("test-vectors.h", "wb");
  if (!f) return 1;

  fprintf(f, "// This is an autogenerated file, generated by gen-test-vectors.c\n");
  fprintf(f, "#ifndef TEST_VECTORS_H\n");
  fprintf(f, "#define TEST_VECTORS_H\n");
  fprintf(f, "#include <stdint.h>\n");

  print_seed(f, "seed_zero", seed_zero);
  print_seed(f, "seed_pi", seed_pi);
  prng_gen(&state, buf, 512);
  print_buffer(f, "shishua_vector_unseeded", buf, sizeof(buf));
  prng_state_half state_half = prng_init_half(seed_zero);

  prng_gen_half(&state_half, buf, 512);
  print_buffer(f, "shishua_half_vector_unseeded", buf, sizeof(buf));
  prng_init(&state, seed_pi);
  prng_gen(&state, buf, 512);
  print_buffer(f, "shishua_vector_seeded", buf, sizeof(buf));

  state_half = prng_init_half(seed_pi);
  prng_gen_half(&state_half, buf, 512);
  print_buffer(f, "shishua_half_vector_seeded", buf, sizeof(buf));

  fprintf(f, "#endif // TEST_VECTORS_H\n");
  fclose(f);
}

