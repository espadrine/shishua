# SHISHUA – The Fastest PRNG In The World

The [announcement and explanation blog post is here][blog post].

_Note: please do not use this for cryptographic purposes._
_If you need security, a recommended option is ChaCha20._

[blog post]: https://espadrine.github.io/blog/posts/shishua-the-fastest-prng-in-the-world.html

Implementations:

- [C](./shishua.h)
- [Rust](https://github.com/dbartussek/shishua_rs/)

## Comparison

<table>
  <tr><th>Name   <th>Performance <th>Quality <th>Seed correlation
  <tr><td>SHISHUA       <td>0.06 <td>>32 TiB <td>>256 GiB
  <tr><td>xoshiro256+x8 <td>0.07 <td>  1 KiB <td>   0 KiB
  <tr><td>RomuTrio      <td>0.31 <td>>32 TiB <td>   1 KiB
  <tr><td>xoshiro256+   <td>0.34 <td>512 MiB <td>   1 KiB
  <tr><td>wyrand        <td>0.41 <td>>32 TiB <td>  32 KiB
  <tr><td>Lehmer128     <td>0.44 <td>>32 TiB <td>   1 KiB
  <tr><td>ChaCha8       <td>0.46 <td>>32 TiB?<td> >32 TiB?
  <tr><td>RC4           <td>8.06 <td>  1 TiB <td>   1 KiB
</table>

1. **Performance**: in number of CPU cycles spent per byte generated,
   on N2 GCP instances. On N2D (AMD), the order is the same.
2. **Quality**: level at which it fails PractRand. We show a `>` if it did not fail.
   We put a question mark if we have not proved it.
3. **Seed correlation**: PractRand on interleaving of bytes from eight streams
   with seeds 1, 2, 4, 8, 16, 32, 64, 128.
   We use PractRand with folding 2 and expanded tests.

On the subject of seed correlations, the `./sample-seed-fingerprints.sh` program
highlights additional anomalies:

- RC4 has weak seeds with heavy artefacts visible in the fingerprints.
- RomuTrio's fingerprints start with the same first character for all seeds.

## Commands

- `make`: build `./shishua`.
- `make test`: run performance tests, PractRand, and BigCrush on SHISHUA.
- `make test/benchmark-seed`: run seed correlation test.
- `make test/benchmark-perf`: run performance comparison locally.
- `make benchmark-intel`: run performance comparison on a GCP Intel chip.
- `make benchmark-amd`: run performance comparison on a GCP AMD chip.

---

The SHISHUA and SHISHUA-half are under the CC0 license.
