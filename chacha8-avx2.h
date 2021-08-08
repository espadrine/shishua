#ifndef CHACHA8_AVX2_H
#define CHACHA8_AVX2_H
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>

// ChaCha8 using SIMD AVX2.
// u8.h: using 8 blocks at a time.
// Inspired by https://github.com/floodyberry/supercop/tree/master/crypto_stream/chacha20/dolbeau/amd64-avx2
// DO NOT USE THIS CODE FOR CRYPTOGRAPHIC PURPOSES.

typedef struct prng_state {
  uint32_t state[16];
} prng_state;

#define ROUNDS 8
#define U8TO32_LITTLE(p) (((uint32_t*)(p))[0])


// SIMD primitives


#define VEC8_ROT(a,imm) _mm256_or_si256(_mm256_slli_epi32(a,imm),_mm256_srli_epi32(a,(32-imm)))

#define VEC8_LINE1(a,b,c,d)                                             \
  x_##a = _mm256_add_epi32(x_##a, x_##b); x_##d = _mm256_shuffle_epi8(_mm256_xor_si256(x_##d, x_##a), rot16)
#define VEC8_LINE2(a,b,c,d)                                             \
  x_##c = _mm256_add_epi32(x_##c, x_##d); x_##b = VEC8_ROT(_mm256_xor_si256(x_##b, x_##c), 12)
#define VEC8_LINE3(a,b,c,d)                                             \
  x_##a = _mm256_add_epi32(x_##a, x_##b); x_##d = _mm256_shuffle_epi8(_mm256_xor_si256(x_##d, x_##a), rot8)
#define VEC8_LINE4(a,b,c,d)                                             \
  x_##c = _mm256_add_epi32(x_##c, x_##d); x_##b = VEC8_ROT(_mm256_xor_si256(x_##b, x_##c),  7)

#define VEC8_ROUND_SEQ(a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,a4,b4,c4,d4)     \
  VEC8_LINE1(a1,b1,c1,d1);                                              \
  VEC8_LINE1(a2,b2,c2,d2);                                              \
  VEC8_LINE1(a3,b3,c3,d3);                                              \
  VEC8_LINE1(a4,b4,c4,d4);                                              \
  VEC8_LINE2(a1,b1,c1,d1);                                              \
  VEC8_LINE2(a2,b2,c2,d2);                                              \
  VEC8_LINE2(a3,b3,c3,d3);                                              \
  VEC8_LINE2(a4,b4,c4,d4);                                              \
  VEC8_LINE3(a1,b1,c1,d1);                                              \
  VEC8_LINE3(a2,b2,c2,d2);                                              \
  VEC8_LINE3(a3,b3,c3,d3);                                              \
  VEC8_LINE3(a4,b4,c4,d4);                                              \
  VEC8_LINE4(a1,b1,c1,d1);                                              \
  VEC8_LINE4(a2,b2,c2,d2);                                              \
  VEC8_LINE4(a3,b3,c3,d3);                                              \
  VEC8_LINE4(a4,b4,c4,d4)

#define VEC8_ROUND(a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,a4,b4,c4,d4) VEC8_ROUND_SEQ(a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,a4,b4,c4,d4)


#define ONEQUAD_UNPCK(a,b,c,d)                                          \
    {                                                                   \
      x_##a = _mm256_add_epi32(x_##a, orig##a);                         \
      x_##b = _mm256_add_epi32(x_##b, orig##b);                         \
      x_##c = _mm256_add_epi32(x_##c, orig##c);                         \
      x_##d = _mm256_add_epi32(x_##d, orig##d);                         \
      t_##a = _mm256_unpacklo_epi32(x_##a, x_##b);                      \
      t_##b = _mm256_unpacklo_epi32(x_##c, x_##d);                      \
      t_##c = _mm256_unpackhi_epi32(x_##a, x_##b);                      \
      t_##d = _mm256_unpackhi_epi32(x_##c, x_##d);                      \
      x_##a = _mm256_unpacklo_epi64(t_##a, t_##b);                      \
      x_##b = _mm256_unpackhi_epi64(t_##a, t_##b);                      \
      x_##c = _mm256_unpacklo_epi64(t_##c, t_##d);                      \
      x_##d = _mm256_unpackhi_epi64(t_##c, t_##d);                      \
    }
#define ONEOCTO(a,b,c,d,a2,b2,c2,d2)                                    \
    {                                                                   \
      ONEQUAD_UNPCK(a,b,c,d);                                           \
      ONEQUAD_UNPCK(a2,b2,c2,d2);                                       \
      t_##a  = _mm256_permute2x128_si256(x_##a, x_##a2, 0x20);          \
      t_##a2 = _mm256_permute2x128_si256(x_##a, x_##a2, 0x31);          \
      t_##b  = _mm256_permute2x128_si256(x_##b, x_##b2, 0x20);          \
      t_##b2 = _mm256_permute2x128_si256(x_##b, x_##b2, 0x31);          \
      t_##c  = _mm256_permute2x128_si256(x_##c, x_##c2, 0x20);          \
      t_##c2 = _mm256_permute2x128_si256(x_##c, x_##c2, 0x31);          \
      t_##d  = _mm256_permute2x128_si256(x_##d, x_##d2, 0x20);          \
      t_##d2 = _mm256_permute2x128_si256(x_##d, x_##d2, 0x31);          \
      _mm256_storeu_si256((__m256i*)(out+  0), t_##a );                  \
      _mm256_storeu_si256((__m256i*)(out+ 64), t_##b );                  \
      _mm256_storeu_si256((__m256i*)(out+128), t_##c );                  \
      _mm256_storeu_si256((__m256i*)(out+192), t_##d );                  \
      _mm256_storeu_si256((__m256i*)(out+256), t_##a2);                  \
      _mm256_storeu_si256((__m256i*)(out+320), t_##b2);                  \
      _mm256_storeu_si256((__m256i*)(out+384), t_##c2);                  \
      _mm256_storeu_si256((__m256i*)(out+448), t_##d2);                  \
    }


// buf's size must be a multiple of 512 bytes.
static inline void prng_gen(prng_state *s, uint8_t out[], size_t bytes) {
  int i;

  if (!bytes || bytes < 512) { return; }

  /* constant for shuffling bytes (replacing multiple-of-8 rotates) */
  __m256i rot16 = _mm256_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2);
  __m256i rot8  = _mm256_set_epi8(14,13,12,15,10,9,8,11,6,5,4,7,2,1,0,3,14,13,12,15,10,9,8,11,6,5,4,7,2,1,0,3);
  uint32_t in12, in13;

  __m256i x_0  = _mm256_set1_epi32(s->state[0]);
  __m256i x_1  = _mm256_set1_epi32(s->state[1]);
  __m256i x_2  = _mm256_set1_epi32(s->state[2]);
  __m256i x_3  = _mm256_set1_epi32(s->state[3]);
  __m256i x_4  = _mm256_set1_epi32(s->state[4]);
  __m256i x_5  = _mm256_set1_epi32(s->state[5]);
  __m256i x_6  = _mm256_set1_epi32(s->state[6]);
  __m256i x_7  = _mm256_set1_epi32(s->state[7]);
  __m256i x_8  = _mm256_set1_epi32(s->state[8]);
  __m256i x_9  = _mm256_set1_epi32(s->state[9]);
  __m256i x_10 = _mm256_set1_epi32(s->state[10]);
  __m256i x_11 = _mm256_set1_epi32(s->state[11]);
  __m256i x_12;// = _mm256_set1_epi32(s->state[12]); /* useless */
  __m256i x_13;// = _mm256_set1_epi32(s->state[13]); /* useless */
  __m256i x_14 = _mm256_set1_epi32(s->state[14]);
  __m256i x_15 = _mm256_set1_epi32(s->state[15]);

  __m256i orig0 = x_0;
  __m256i orig1 = x_1;
  __m256i orig2 = x_2;
  __m256i orig3 = x_3;
  __m256i orig4 = x_4;
  __m256i orig5 = x_5;
  __m256i orig6 = x_6;
  __m256i orig7 = x_7;
  __m256i orig8 = x_8;
  __m256i orig9 = x_9;
  __m256i orig10 = x_10;
  __m256i orig11 = x_11;
  __m256i orig12;// = x_12; /* useless */
  __m256i orig13;// = x_13; /* useless */
  __m256i orig14 = x_14;
  __m256i orig15 = x_15;
  __m256i t_0;
  __m256i t_1;
  __m256i t_2;
  __m256i t_3;
  __m256i t_4;
  __m256i t_5;
  __m256i t_6;
  __m256i t_7;
  __m256i t_8;
  __m256i t_9;
  __m256i t_10;
  __m256i t_11;
  __m256i t_12;
  __m256i t_13;
  __m256i t_14;
  __m256i t_15;


  while (bytes >= 512) {
    x_0 = orig0;
    x_1 = orig1;
    x_2 = orig2;
    x_3 = orig3;
    x_4 = orig4;
    x_5 = orig5;
    x_6 = orig6;
    x_7 = orig7;
    x_8 = orig8;
    x_9 = orig9;
    x_10 = orig10;
    x_11 = orig11;
    //x_12 = orig12; /* useless */
    //x_13 = orig13; /* useless */
    x_14 = orig14;
    x_15 = orig15;

    // Increment the counter.

    const __m256i addv12 = _mm256_set_epi64x(3,2,1,0);
    const __m256i addv13 = _mm256_set_epi64x(7,6,5,4);
    const __m256i permute = _mm256_set_epi32(7,6,3,2,5,4,1,0);
    __m256i t12, t13;
    in12 = s->state[12];
    in13 = s->state[13];
    uint64_t in1213 = ((uint64_t)in12) | (((uint64_t)in13) << 32);
    x_12 = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(in1213));
    x_13 = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(in1213));

    t12 = _mm256_add_epi64(addv12, x_12);
    t13 = _mm256_add_epi64(addv13, x_13);

    x_12 = _mm256_unpacklo_epi32(t12, t13);
    x_13 = _mm256_unpackhi_epi32(t12, t13);

    t12 = _mm256_unpacklo_epi32(x_12, x_13);
    t13 = _mm256_unpackhi_epi32(x_12, x_13);

    /* required because unpack* are intra-lane */
    x_12 = _mm256_permutevar8x32_epi32(t12, permute);
    x_13 = _mm256_permutevar8x32_epi32(t13, permute);

    orig12 = x_12;
    orig13 = x_13;

    in1213 += 8;

    s->state[12] = in1213 & 0xFFFFFFFF;
    s->state[13] = (in1213>>32)&0xFFFFFFFF;

    // Hash the counter.

    for (i = 0 ; i < ROUNDS ; i+=2) {
      VEC8_ROUND( 0, 4, 8,12, 1, 5, 9,13, 2, 6,10,14, 3, 7,11,15);
      VEC8_ROUND( 0, 5,10,15, 1, 6,11,12, 2, 7, 8,13, 3, 4, 9,14);
    }

    ONEOCTO(0,1,2,3,4,5,6,7);
    out+=32;
    ONEOCTO(8,9,10,11,12,13,14,15);
    out-=32;


    bytes -= 512;
    out += 512;

    // We do whatever remains the normal way.
    if (!bytes) return;
  }
}

static const char sigma[16] = "expand 32-byte k";

void prng_init(prng_state *s, uint64_t seed[4]) {
  // Constant.
  s->state[ 0] = U8TO32_LITTLE(sigma + 0);
  s->state[ 1] = U8TO32_LITTLE(sigma + 4);
  s->state[ 2] = U8TO32_LITTLE(sigma + 8);
  s->state[ 3] = U8TO32_LITTLE(sigma + 12);

  // Key. I ignore the little-endian details here as they don't affect speed.
  s->state[ 4] = seed[0] & 0xffffffff;
  s->state[ 5] = seed[0] >> 32;
  s->state[ 6] = seed[1] & 0xffffffff;
  s->state[ 7] = seed[1] >> 32;
  s->state[ 8] = seed[2] & 0xffffffff;
  s->state[ 9] = seed[2] >> 32;
  s->state[10] = seed[3] & 0xffffffff;
  s->state[11] = seed[3] >> 32;

  // IV. We don't put an IV. We are not doing crypto here.
  s->state[12] = 0;
  s->state[13] = 0;
  s->state[14] = 0;
  s->state[15] = 0;
}
#endif
