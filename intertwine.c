#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFSIZE (1<<17)

typedef struct args { int rval; int blockSize; int fnum; int *files; } args_t;
args_t parseArgs(int argc, char **argv);

int main(int argc, char **argv) {
  args_t a = parseArgs(argc, argv);
  if (a.rval < 0) { return a.rval; }
  int outputBufsize = BUFSIZE * a.fnum;
  char ibuf[BUFSIZE];
  char *obuf = malloc(outputBufsize * sizeof(char));
  char reachedEOF = 0;
  while (!reachedEOF) {
    int readBytes = outputBufsize;
    for (int i = 0; i < a.fnum; i++) {
      ssize_t r = read(a.files[i], ibuf, BUFSIZE);
      for (int bi = 0; bi < r; bi++) {
        obuf[a.fnum * bi + i] = ibuf[bi];
      }
      if (r < BUFSIZE) {
        if (readBytes > a.fnum * r + i) {
          readBytes = a.fnum * r + i;
        }
      }
      if (r == 0) {
        reachedEOF = 1;
      }
    }
    ssize_t w = write(STDOUT_FILENO, obuf, readBytes);
  }

  // Discharging allocations.
  for (int i = 0; i < a.fnum; i++) { close(a.files[i]); }
  free(a.files); free(obuf);
  return 0;
}

args_t parseArgs(int argc, char **argv) {
  args_t a;
  a.rval = 0;
  a.blockSize = 1;
  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      fprintf(stderr, "Usage: intertwine [args] <FILE1> <FILE2>\n");
      fprintf(stderr, "Intertwine blocks from two sources.\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "  --block-size: as bytes (default 1).\n");
      a.rval = -1;
      continue;
    } else if (strcmp(argv[i], "--block-size") == 0) {
      a.blockSize = atoll(argv[++i]);
      continue;
    } else if (strcmp(argv[i], "--") == 0) {
      i++;
      break;
    } else if (argv[i][0] != '-') {
      break;
    }
  }

  // Read the files.
  a.fnum = argc - i;
  a.files = malloc(a.fnum * sizeof(int));
  for (int j = i; j < argc; j++) {
    a.files[j - i] = open(argv[j], O_RDONLY);
  }

  return a;
}
