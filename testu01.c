#include <stdio.h>
#include <string.h>
#include "TestU01.h"

static const int SMALL = 1, MEDIUM = 2, BIG = 3;

int parseArgs(int argc, char **argv) {
  int size = MEDIUM;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      fprintf(stderr, "Usage: testu01 [args]\n");
      fprintf(stderr, "Analyze the statistical quality of PRNGs.\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "  --small: SmallCrush, a small and fast battery of 10 tests. 500 MB. 30s.\n");
      fprintf(stderr, "  --medium: Crush (default), a suite of 96 stringent tests. 1 TB. 1h.\n");
      fprintf(stderr, "  --big: BigCrush, a suite of 106 very stringent tests. 8 TB. 8h.\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "User guide: <http://simul.iro.umontreal.ca/testu01/guideshorttestu01.pdf>\n");
      return -1;
    } else if (strcmp(argv[i], "--small") == 0) {
      size = SMALL;
    } else if (strcmp(argv[i], "--medium") == 0) {
      size = MEDIUM;
    } else if (strcmp(argv[i], "--big") == 0) {
      size = BIG;
    }
  }
  return size;
}

int main(int argc, char **argv) {
  int size = parseArgs(argc, argv);
  if (size < SMALL) { return size; }
  unif01_Gen* gen = ufile_CreateReadBin("/dev/stdin", 16384);

  if      (size == SMALL ) { bbattery_SmallCrush(gen); }
  else if (size == MEDIUM) { bbattery_Crush(gen); }
  else if (size == BIG   ) { bbattery_BigCrush(gen); }

  ufile_DeleteReadBin(gen);
  return 0;
}
