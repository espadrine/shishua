#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct timespec prng_timer_t;

static inline prng_timer_t timer_start(void) {
   struct timespec ret;
   clock_gettime(CLOCK_REALTIME, &ret);
   return ret;
}
// Converts a struct timespec to nanoseconds
static inline int64_t timespec_to_ns(struct timespec spec) {
  return (int64_t)spec.tv_nsec + ((int64_t)spec.tv_sec * (int64_t)1e9);
}
static inline int64_t timer_elapsed(prng_timer_t start) {
  struct timespec end = timer_start();
  int64_t start_ns = timespec_to_ns(start);
  int64_t end_ns = timespec_to_ns(end);
  // Measure the latency of clock_gettime.
  // Since it is sensitive to clock speed and such, we measure each time.
  int64_t latency = 0;
  const int LATENCY_ROUNDS = 64;
  // sample 64 times and get the average.
  for (int i = 0; i < LATENCY_ROUNDS; i++) {
    struct timespec a = timer_start();
    struct timespec b = timer_start();
    latency += timespec_to_ns(b) - timespec_to_ns(a);
  }
  latency /= LATENCY_ROUNDS;
  return end_ns - start_ns - latency;
}

// Use cycle count if possible
#if !defined(PRNG_USE_NANOSECONDS) && (defined(__x86_64__) || defined(__i386__))
#  include <x86intrin.h>
static const char *unit = "cpb";
typedef int64_t prng_cycle_t;

static inline prng_cycle_t cycle_counter_start(void) {
  return _rdtsc();
}
static inline int64_t cycle_counter_elapsed(prng_cycle_t start) {
  int64_t end = _rdtsc();
  return end - start;
}
#else
// fall back to nanoseconds, e.g. on aarch64 where __builtin_readcyclecounter
// needs special privileges not usually granted by the Linux kernel.
// Note that this depends on the CPU frequency.
// This assumes that clock_gettime is available.
static const char *unit = "ns/byte";
typedef struct timespec prng_cycle_t;
static inline prng_cycle_t cycle_counter_start(void) {
  return timer_start();
}
static inline int64_t cycle_counter_elapsed(prng_cycle_t start) {
  return timer_elapsed(start);
}
#endif

#define BUFSIZE (1<<17)
#ifndef HEADER
#  define HEADER "./prng.h"
#endif
#include HEADER

typedef struct args { int64_t bytes; uint64_t seed[4]; int rval; int quiet; } args_t;
args_t parseArgs(int argc, char **argv);
void parseSeed(char *seed, args_t *args);

int main(int argc, char **argv) {
  args_t a = parseArgs(argc, argv);
  if (a.rval < 0) { return a.rval; }
  prng_state s;
  prng_init(&s, a.seed);
  uint8_t buf[BUFSIZE] __attribute__ ((aligned (64)));
  int64_t cycles = 0, ns;
  prng_cycle_t cycles_start;
  prng_timer_t time_start = timer_start();
  for (int64_t bytes = a.bytes; bytes >= 0; bytes -= sizeof(buf)) {
    int wbytes = bytes < sizeof(buf)? bytes: sizeof(buf);
    cycles_start = cycle_counter_start();
    prng_gen(&s, buf, sizeof(buf));
    cycles += cycle_counter_elapsed(cycles_start);
    if (!a.quiet) {
      ssize_t w = write(STDOUT_FILENO, buf, wbytes);
    }
  }
  ns = timer_elapsed(time_start);
  fprintf(stderr, "%-20s\t%f %s\t%.2f GB/s\n",
          HEADER, ((double)cycles) / a.bytes, unit,
          ((double)a.bytes) / ns);
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
      fprintf(stderr, "  --quiet: don't dump output to stdout\n");
      a.rval = -1;
    } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bytes") == 0) {
      a.bytes = strtoll(argv[++i], NULL, 10);
      if (a.bytes <= 0) a.rval = -1;
    } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
      a.quiet = 1;
    } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--seed") == 0) {
      parseSeed(argv[++i], &a);
    }
  }
  return a;
}

void parseSeed(char *seed, args_t *args) {
  size_t consumed = 0;
  size_t len = strlen(seed);
  for (int i = 0; i < 4; i++) {
    // 16 hex chars (64 bits / 4 bit/hex digit) + 1 \0.
    char hex64[17] = "0000000000000000";
    //  consumed hexLen
    // |xxxxxxxx|xxxx--|   x = len
    size_t hexLen = len - consumed;
    if (hexLen > 16) { hexLen = 16; }
    strncpy(hex64, seed + i*16, hexLen);
    consumed += hexLen;
    hex64[16] = '\0';
    args->seed[i] = strtoull(hex64, NULL, 16);
  }
}
