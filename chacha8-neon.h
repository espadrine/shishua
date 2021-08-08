#ifndef CHACHA8_NEON_H
#define CHACHA8_NEON_H
#include <arm_neon.h>
#include <stdio.h>
#include <stdint.h>

// ChaCha8 using SIMD NEON.
// u4.h: using 4 blocks at a time (4×32bits = 128-bit = a NEON register).
// Inspired by https://github.com/floodyberry/supercop/blob/master/crypto_stream/chacha20/dolbeau/arm-neon/chacha.c
// DO NOT USE THIS CODE FOR CRYPTOGRAPHIC PURPOSES.

typedef struct prng_state {
  uint32_t state[16];
} prng_state;

#define ROUNDS 8
#define U8TO32_LITTLE(p) (((uint32_t*)(p))[0])


// SIMD primitives


/* in <http://cryptosith.org/papers/neoncrypto-20120320.pdf>,
   Bernstein & Schwabe say the two-instructions rotation
   (using shift-with-insert) is slightly worse on Cortex A8
   and/or A9. It seems better on my Cortex A15
   (for this code using my gcc, anyway) */
#define VEC4_ROT(a,imm) vsliq_n_u32(vshrq_n_u32(a, 32-imm),a,imm)

#define VEC4_ROT16(a) vreinterpretq_u32_u16(vrev32q_u16(vreinterpretq_u16_u32(a)))

#define VEC4_QUARTERROUND(a,b,c,d)                                \
   x_##a = vaddq_u32(x_##a, x_##b); t_##a = veorq_u32(x_##d, x_##a); x_##d = VEC4_ROT16(t_##a); \
   x_##c = vaddq_u32(x_##c, x_##d); t_##c = veorq_u32(x_##b, x_##c); x_##b = VEC4_ROT(t_##c, 12); \
   x_##a = vaddq_u32(x_##a, x_##b); t_##a = veorq_u32(x_##d, x_##a); x_##d = VEC4_ROT(t_##a,  8); \
   x_##c = vaddq_u32(x_##c, x_##d); t_##c = veorq_u32(x_##b, x_##c); x_##b = VEC4_ROT(t_##c,  7)


// buf's size must be a multiple of 256 bytes.
static inline void prng_gen(prng_state *s, uint8_t out[], size_t bytes) {
  int i;

  if (!bytes || bytes < 256) { return; }

  uint32_t in12, in13;
  uint32x4_t x_0 = vdupq_n_u32(s->state[0]);
  uint32x4_t x_1 = vdupq_n_u32(s->state[1]);
  uint32x4_t x_2 = vdupq_n_u32(s->state[2]);
  uint32x4_t x_3 = vdupq_n_u32(s->state[3]);
  uint32x4_t x_4 = vdupq_n_u32(s->state[4]);
  uint32x4_t x_5 = vdupq_n_u32(s->state[5]);
  uint32x4_t x_6 = vdupq_n_u32(s->state[6]);
  uint32x4_t x_7 = vdupq_n_u32(s->state[7]);
  uint32x4_t x_8 = vdupq_n_u32(s->state[8]);
  uint32x4_t x_9 = vdupq_n_u32(s->state[9]);
  uint32x4_t x_10 = vdupq_n_u32(s->state[10]);
  uint32x4_t x_11 = vdupq_n_u32(s->state[11]);
  uint32x4_t x_12;// = vdupq_n_u32(s->state[12]); /* useless */
  uint32x4_t x_13;// = vdupq_n_u32(s->state[13]); /* useless */
  uint32x4_t x_14 = vdupq_n_u32(s->state[14]);
  uint32x4_t x_15 = vdupq_n_u32(s->state[15]);
  uint32x4_t orig0 = x_0;
  uint32x4_t orig1 = x_1;
  uint32x4_t orig2 = x_2;
  uint32x4_t orig3 = x_3;
  uint32x4_t orig4 = x_4;
  uint32x4_t orig5 = x_5;
  uint32x4_t orig6 = x_6;
  uint32x4_t orig7 = x_7;
  uint32x4_t orig8 = x_8;
  uint32x4_t orig9 = x_9;
  uint32x4_t orig10 = x_10;
  uint32x4_t orig11 = x_11;
  uint32x4_t orig12;// = x_12; /* useless */
  uint32x4_t orig13;// = x_13; /* useless */
  uint32x4_t orig14 = x_14;
  uint32x4_t orig15 = x_15;
  uint32x4_t t_0;
  uint32x4_t t_1;
  uint32x4_t t_2;
  uint32x4_t t_3;
  uint32x4_t t_4;
  uint32x4_t t_5;
  uint32x4_t t_6;
  uint32x4_t t_7;
  uint32x4_t t_8;
  uint32x4_t t_9;
  uint32x4_t t_10;
  uint32x4_t t_11;
  uint32x4_t t_12;
  uint32x4_t t_13;
  uint32x4_t t_14;
  uint32x4_t t_15;

  while (bytes >= 256) {
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



    const uint64x2_t addv12 = vcombine_u64(vcreate_u64(2),vcreate_u64(3));
    const uint64x2_t addv13 = vcombine_u64(vcreate_u64(0),vcreate_u64(1));
    uint64x2_t t12, t13;
    in12 = s->state[12];
    in13 = s->state[13];
    uint64_t in1213 = ((uint64_t)in12) | (((uint64_t)in13) << 32);
    t12 = vdupq_n_u64(in1213);
    t13 = vdupq_n_u64(in1213);

    x_12 = vreinterpretq_u32_u64(vaddq_u64(addv12, t12));
    x_13 = vreinterpretq_u32_u64(vaddq_u64(addv13, t13));

    uint32x4x2_t t;
    t = vuzpq_u32(x_13,x_12);
    x_12 = t.val[0];
    x_13 = t.val[1];

    orig12 = x_12;
    orig13 = x_13;

    in1213 += 4;

    s->state[12] = in1213 & 0xFFFFFFFF;
    s->state[13] = (in1213>>32)&0xFFFFFFFF;

    for (i = 0 ; i < ROUNDS ; i+=2) {
      VEC4_QUARTERROUND( 0, 4, 8,12);
      VEC4_QUARTERROUND( 1, 5, 9,13);
      VEC4_QUARTERROUND( 2, 6,10,14);
      VEC4_QUARTERROUND( 3, 7,11,15);
      VEC4_QUARTERROUND( 0, 5,10,15);
      VEC4_QUARTERROUND( 1, 6,11,12);
      VEC4_QUARTERROUND( 2, 7, 8,13);
      VEC4_QUARTERROUND( 3, 4, 9,14);
    }

#define ONEQUAD_TRANSPOSE(a,b,c,d)                                      \
    {                                                                   \
      uint32x4x2_t t0dq, t1dq;                                          \
      x_##a = vaddq_u32(x_##a, orig##a);                                \
      x_##b = vaddq_u32(x_##b, orig##b);                                \
      x_##c = vaddq_u32(x_##c, orig##c);                                \
      x_##d = vaddq_u32(x_##d, orig##d);                                \
      t0dq = vtrnq_u32(x_##a,x_##b);                                      \
      t1dq = vtrnq_u32(x_##c,x_##d);                                      \
      x_##a = vreinterpretq_u32_u64(vcombine_u64(vget_low_u64(vreinterpretq_u64_u32(t0dq.val[0])), vget_low_u64(vreinterpretq_u64_u32(t1dq.val[0])))); \
      x_##b = vreinterpretq_u32_u64(vcombine_u64(vget_low_u64(vreinterpretq_u64_u32(t0dq.val[1])), vget_low_u64(vreinterpretq_u64_u32(t1dq.val[1])))); \
      x_##c = vreinterpretq_u32_u64(vcombine_u64(vget_high_u64(vreinterpretq_u64_u32(t0dq.val[0])), vget_high_u64(vreinterpretq_u64_u32(t1dq.val[0])))); \
      x_##d = vreinterpretq_u32_u64(vcombine_u64(vget_high_u64(vreinterpretq_u64_u32(t0dq.val[1])), vget_high_u64(vreinterpretq_u64_u32(t1dq.val[1])))); \
      vst1q_u32((uint32_t*)(out+0),x_##a);                              \
      vst1q_u32((uint32_t*)(out+64),x_##b);                             \
      vst1q_u32((uint32_t*)(out+128),x_##c);                            \
      vst1q_u32((uint32_t*)(out+192),x_##d);                            \
    }

#define ONEQUAD(a,b,c,d) ONEQUAD_TRANSPOSE(a,b,c,d)

    ONEQUAD(0,1,2,3);
    out+=16;
    ONEQUAD(4,5,6,7);
    out+=16;
    ONEQUAD(8,9,10,11);
    out+=16;
    ONEQUAD(12,13,14,15);
    out-=48;

#undef ONEQUAD
#undef ONEQUAD_TRANSPOSE

    bytes -= 256;
    out += 256;
  }
}
#undef VEC4_ROT
#undef VEC4_QUARTERROUND

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
#endif // CHACHA8_NEON_H
