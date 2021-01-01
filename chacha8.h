#ifndef CHACHA8_H
#define CHACHA8_H

#define CHACHA8_TARGET_SCALAR 0
#define CHACHA8_TARGET_AVX2   1
#define CHACHA8_TARGET_NEON   3

#ifndef CHACHA8_TARGET
#  if defined(__AVX2__) && (defined(__x86_64__) || defined(_M_X64))
#    define CHACHA8_TARGET CHACHA8_TARGET_AVX2
#  elif (defined(__ARM_NEON) || defined(__ARM_NEON__))
#    define CHACHA8_TARGET CHACHA8_TARGET_NEON
#  else
#    define CHACHA8_TARGET CHACHA8_TARGET_SCALAR
#  endif
#endif

// These are all optional, with defining CHACHA8_TARGET_SCALAR, you only
// need this header.
#if CHACHA8_TARGET == CHACHA8_TARGET_AVX2
#  include "chacha8-avx2.h"
#elif CHACHA8_TARGET == CHACHA8_TARGET_NEON
#  include "chacha8-neon.h"
#else // CHACHA8_TARGET == CHACHA8_TARGET_SCALAR

// FIXME: Implement the scalar version.
# error "CPU not supported"

#endif // CHACHA8_TARGET == CHACHA8_TARGET_SCALAR
#endif // CHACHA8_H
