# SHISHUA – The Fastest PRNG In The World

The [announcement and explanation blog post is here][blog post].

_Note: please do not use this for cryptographic purposes._
_If you need security, a recommended option is ChaCha20._

[blog post]: https://espadrine.github.io/blog/posts/shishua-the-fastest-prng-in-the-world.html

Implementations:

- [C](./shishua.h)
- [Rust](https://github.com/dbartussek/shishua_rs/)
- [Node.js](https://github.com/espadrine/shishua-nodejs) (on [npm](https://www.npmjs.com/package/shishua))
- [Python](https://github.com/espadrine/shishua-python) (on [PyPI](https://pypi.org/project/shishua/))

## Comparison

<table>
  <tr><th>Name <th><a href='./test/benchmark-perf-intel'>Intel Performance</a> <th><a href='./test/benchmark-perf-arm'>ARM Performance</a> <th>Quality <th>Seed correlation
  <tr><td>SHISHUA       <td>52.93 GB/s <td> 8.78 GB/s <td> >32 TiB  <td> >32 TiB
  <tr><td>xoshiro256+x8 <td>40.18 GB/s <td> 3.69 GB/s <td>   1 KiB  <td>   0 KiB
  <tr><td>RomuTrio      <td> 8.62 GB/s <td> 4.76 GB/s <td> >32 TiB  <td>   1 KiB
  <tr><td>xoshiro256+   <td> 7.88 GB/s <td> 4.31 GB/s <td> 512 MiB  <td>   1 KiB
  <tr><td>wyrand        <td> 7.00 GB/s <td> 2.53 GB/s <td> >32 TiB  <td>  32 KiB
  <tr><td>Lehmer128     <td> 6.14 GB/s <td> 1.83 GB/s <td> >32 TiB  <td>   1 KiB
  <tr><td>ChaCha8       <td> 6.28 GB/s <td> 1.72 GB/s <td> >32 TiB? <td> >32 TiB?
  <tr><td>RC4           <td> 0.35 GB/s <td> 0.15 GB/s <td>   1 TiB  <td>   1 KiB
</table>

1. **Performance**: in number of CPU cycles spent per byte generated,
   on N2 GCP instances. On N2D (AMD), the order is the same.
2. **Quality**: level at which it fails PractRand. We show a `>` if it did not fail.
   We put a question mark if we have not proved it.
3. **Seed correlation**: PractRand on interleaving of bytes from eight streams
   with seeds 1, 2, 4, 8, 16, 32, 64, 128.
   We use PractRand with folding 2 and expanded tests.

On the subject of seed correlations, the `./bin/sample-seed-fingerprints.sh` program
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
- `make benchmark-arm`: run performance comparison on an AWS Graviton ARM chip.

---

The SHISHUA and SHISHUA-half are under the CC0 license.
