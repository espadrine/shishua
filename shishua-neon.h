// An ARM NEON version of shishua.
#ifndef SHISHUA_NEON_H
#define SHISHUA_NEON_H
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <arm_neon.h>

typedef struct prng_state {
  uint64x2_t state[8];
  uint64x2_t output[8];
  uint64x2_t counter[2];
} prng_state;

#if defined(__GNUC__) && (defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__)
#  define SHISHUA_VSETQ_N_U64(a, b) (__extension__(uint64x2_t) { a, b })
#else
#  define SHISHUA_VSETQ_N_U64(a, b) vcombine_u64(vdup_n_u64(a), vdup_n_u64(b))
#endif
// To hide the vreinterpret ritual.
#define SHISHUA_VEXTQ_U8(Rn, Rm, Imm) \
  vreinterpretq_u64_u8( \
    vextq_u8( \
      vreinterpretq_u8_u64(Rn), \
      vreinterpretq_u8_u64(Rm), \
      (Imm) \
    ) \
  )

// buf could technically alias with prng_state, according to the compiler.
#if defined(__GNUC__) || defined(_MSC_VER)
#  define SHISHUA_RESTRICT __restrict
#else
#  define SHISHUA_RESTRICT
#endif

// buf's size must be a multiple of 128 bytes.
static inline void prng_gen(prng_state *SHISHUA_RESTRICT s, uint8_t *SHISHUA_RESTRICT buf, size_t size) {
  uint8_t *b = buf;
  uint64x2_t counter_lo = s->counter[0], counter_hi = s->counter[1];
  // The counter is not necessary to beat PractRand.
  // It sets a lower bound of 2^71 bytes = 2 ZiB to the period,
  // or about 7 millenia at 10 GiB/s.
  // The increments are picked as odd numbers,
  // since only coprimes of the base cover the full cycle,
  // and all odd numbers are coprime of 2.
  // I use different odd numbers for each 64-bit chunk
  // for a tiny amount of variation stirring.
  // I used the smallest odd numbers to avoid having a magic number.
  uint64x2_t increment_lo = SHISHUA_VSETQ_N_U64(7, 5);
  uint64x2_t increment_hi = SHISHUA_VSETQ_N_U64(3, 1);
  // TODO: consider adding proper uneven write handling
  assert((size % 128 == 0) && "buf's size must be a multiple of 128 bytes.");

  for (size_t i = 0; i < size; i += 128) {
    // Write the current output block to state if it is not NULL
    if (buf != NULL) {
      for (size_t j = 0; j < 8; j++) {
        vst1q_u8(b, vreinterpretq_u8_u64(s->output[j]));
        b += 16;
      }
    }
    // NEON has less register pressure than SSE2, but we reroll it anyways for
    // code size.
    for (size_t j = 0; j < 2; j++) {
      uint64x2_t s0_lo = s->state[j * 4 + 0],
                 s0_hi = s->state[j * 4 + 1],
                 s1_lo = s->state[j * 4 + 2],
                 s1_hi = s->state[j * 4 + 3],
                 t0_lo, t0_hi, t1_lo, t1_hi,
                 u_lo, u_hi;

      // I apply the counter to s1,
      // since it is the one whose shift loses most entropy.
      s1_lo = vaddq_u64(s1_lo, counter_lo); s1_hi = vaddq_u64(s1_hi, counter_hi);

      // The following shuffles move weak (low-diffusion) 32-bit parts of 64-bit
      // additions to strong positions for enrichment. The low 32-bit part of a
      // 64-bit chunk never moves to the same 64-bit chunk as its high part.
      // They do not remain in the same chunk. Each part eventually reaches all
      // positions ringwise: A to B, B to C, …, H to A.
      // You may notice that they are simply 256-bit rotations (96 and 160).
      // Note: This:
      //   x = (y << 96) | (y >> 160)
      // can be rewritten as this
      //   x_lo = (y_lo << 96) | (y_hi >> 32)
      //   x_hi = (y_hi << 96) | (y_lo >> 32)
      // which we can do with 2 vext.8 instructions.
      t0_lo = SHISHUA_VEXTQ_U8(s0_hi, s0_lo, 4);   t0_hi = SHISHUA_VEXTQ_U8(s0_lo, s0_hi,  4);
      t1_lo = SHISHUA_VEXTQ_U8(s1_lo, s1_hi, 12);  t1_hi = SHISHUA_VEXTQ_U8(s1_hi, s1_lo, 12);

      // SIMD does not support rotations. Shift is the next best thing to entangle
      // bits with other 64-bit positions. We must shift by an odd number so that
      // each bit reaches all 64-bit positions, not just half. We must lose bits
      // of information, so we minimize it: 1 and 3. We use different shift values
      // to increase divergence between the two sides. We use rightward shift
      // because the rightmost bits have the least diffusion in addition (the low
      // bit is just a XOR of the low bits).
      u_lo = vshrq_n_u64(s0_lo, 1);   u_hi = vshrq_n_u64(s0_hi, 1);
#if defined(__clang__)
      // UGLY HACK: Clang enjoys merging the above statements with the vadds below
      // into vsras.
      // This is dumb, as it still needs to do the original vshr for the xor mix,
      // causing it to shift twice.
      // This makes Clang assume that this line has side effects, preventing the
      // combination and speeding things up significantly.
      __asm__("" : "+w" (u_lo), "+w" (u_hi));
#endif

      // Addition is the main source of diffusion.
      // Storing the output in the state keeps that diffusion permanently.
      s->state[4 * j + 0] = vaddq_u64(t0_lo, u_lo);
      s->state[4 * j + 1] = vaddq_u64(t0_hi, u_hi);
      // Use vsra here directly.
      s->state[4 * j + 2] = vsraq_n_u64(t1_lo, s1_lo, 3);
      s->state[4 * j + 3] = vsraq_n_u64(t1_hi, s1_hi, 3);

      // The first orthogonally grown pieces evolving independently, XORed.
      s->output[2 * j + 0] = veorq_u64(u_lo, t1_lo);
      s->output[2 * j + 1] = veorq_u64(u_hi, t1_hi);

    }
    // The second orthogonally grown piece evolving independently, XORed.
    s->output[4] = veorq_u64(s->state[0], s->state[6]);
    s->output[5] = veorq_u64(s->state[1], s->state[7]);

    s->output[6] = veorq_u64(s->state[2], s->state[4]);
    s->output[7] = veorq_u64(s->state[3], s->state[5]);

    counter_lo = vaddq_u64(counter_lo, increment_lo);
    counter_hi = vaddq_u64(counter_hi, increment_hi);
  }
  s->counter[0] = counter_lo;
  s->counter[1] = counter_hi;
}

// Nothing up my sleeve: those are the hex digits of Φ,
// the least approximable irrational number.
// $ echo 'scale=310;obase=16;(sqrt(5)-1)/2' | bc
static uint64_t phi[16] = {
  0x9E3779B97F4A7C15, 0xF39CC0605CEDC834, 0x1082276BF3A27251, 0xF86C6A11D0C18E95,
  0x2767F0B153D27B7F, 0x0347045B5BF1827F, 0x01886F0928403002, 0xC1D64BA40F335E36,
  0xF06AD7AE9717877E, 0x85839D6EFFBD7DC6, 0x64D325D1C5371682, 0xCADD0CCCFDFFBBE1,
  0x626E33B8D04B4331, 0xBBF73C790D94F79D, 0x471C4AB3ED3D82A5, 0xFEC507705E4AE6E5,
};

void prng_init(prng_state *s, uint64_t seed[4]) {
  s->counter[0] = vdupq_n_u64(0);
  s->counter[1] = vdupq_n_u64(0);
# define ROUNDS 13
# define STEPS 1
  // Diffuse first two seed elements in s0, then the last two. Same for s1.
  // We must keep half of the state unchanged so users cannot set a bad state.
  uint64x2_t seed_0 = SHISHUA_VSETQ_N_U64(seed[0], 0);
  uint64x2_t seed_1 = SHISHUA_VSETQ_N_U64(seed[1], 0);
  uint64x2_t seed_2 = SHISHUA_VSETQ_N_U64(seed[2], 0);
  uint64x2_t seed_3 = SHISHUA_VSETQ_N_U64(seed[3], 0);
  s->state[0] = veorq_u64(seed_0, vld1q_u64(&phi[ 0]));
  s->state[1] = veorq_u64(seed_1, vld1q_u64(&phi[ 2]));
  s->state[2] = veorq_u64(seed_2, vld1q_u64(&phi[ 4]));
  s->state[3] = veorq_u64(seed_3, vld1q_u64(&phi[ 6]));
  s->state[4] = veorq_u64(seed_2, vld1q_u64(&phi[ 8]));
  s->state[5] = veorq_u64(seed_3, vld1q_u64(&phi[10]));
  s->state[6] = veorq_u64(seed_0, vld1q_u64(&phi[12]));
  s->state[7] = veorq_u64(seed_1, vld1q_u64(&phi[14]));

  for (int i = 0; i < ROUNDS; i++) {
    prng_gen(s, NULL, 128 * STEPS);
    s->state[0] = s->output[6];   s->state[1] = s->output[7];
    s->state[2] = s->output[4];   s->state[3] = s->output[5];
    s->state[4] = s->output[2];   s->state[5] = s->output[3];
    s->state[6] = s->output[0];   s->state[7] = s->output[1];
  }
# undef STEPS
# undef ROUNDS
}
#undef SHISHUA_VSETQ_N_U64
#undef SHISHUA_VEXTQ_U8
#undef SHISHUA_RESTRICT
#endif
