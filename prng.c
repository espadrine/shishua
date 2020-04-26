#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>
#define BUFSIZE (1<<14)
#define SEEDTYPE uint64_t
#include "./prng.h"
typedef struct args { int64_t bytes; SEEDTYPE seed[4]; int rval; } args_t;
args_t parseArgs(int argc, char **argv);

int main(int argc, char **argv) {
  args_t a = parseArgs(argc, argv);
  if (a.rval < 0) { return a.rval; }
  prng_state s = prng_init(a.seed);
  uint64_t buf[BUFSIZE] __attribute__ ((aligned (64)));
  int64_t cycles = 0, start;
  for (int64_t bytes = a.bytes; bytes >= 0; bytes -= sizeof(buf)) {
    int wbytes = bytes < sizeof(buf)? bytes: sizeof(buf);
    start = _rdtsc();
    prng_gen(&s, buf, BUFSIZE);
    cycles += _rdtsc() - start;
    ssize_t w = write(STDOUT_FILENO, buf, wbytes);
  }
  fprintf(stderr, "%f cpb\n", ((double)cycles) / a.bytes);
  return 0;
}

args_t parseArgs(int argc, char **argv) {
  args_t a = {0};
  a.rval = 0;
  a.bytes = 0x7fffffffffffffff;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      fprintf(stderr, "Usage: prng [args]\n");
      fprintf(stderr, "A PRNG.\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "  --bytes: as bytes.\n");
      fprintf(stderr, "  --seed: as hexadecimal.\n");
      a.rval = -1;
    } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bytes") == 0) {
      a.bytes = strtoll(argv[++i], NULL, 0);
    } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--seed") == 0) {
      a.seed[0] = strtoull(argv[++i], NULL, 0);
    }
  }
  return a;
}
