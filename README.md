# PCG64 (pcg_state_oneseq_128 XSL-RR) C implementation for 32 / 64-bit platforms

## What's this?

[PCG (Permuted Congruential Generator)](https://www.pcg-random.org/index.html) Pseudo random number generator implementation of C (C99).

This code was created to implement the Random extension in PHP.

It includes an implementation that improves the portability of the original PCG (state: 128-bit, generate: 64-bit) so that it can run on 32-bit platforms.

It aims to be an implementation of Rust's `Pcg64` and NumPy's `PCG64`.

## Limitations

- Not possible to seed with 128-bit values
    - Because it is a PoC. PHP implementation allows 128-bit seeding with string.
- Cannot fully testing on `__uint128_t` unsupported environment

## Clone

```shell
$ git clone --recursive https://github.com/zeriyoshi/pcg64_example.git "pcg64_example"
```

## Build

`reference_implementation` requires `__uint128_t` support

```shell
$ make all
```

## Test

### Baremetal

#### 32-bit without `__uint128_t` support

```shell
$ make test32
```

#### 64-bit with `__uint128_t` support

```shell
$ make test
```

### Virtual

#### 32-bit without `__uint128_t` support

requires Docker (with QEMU) and `docker-compose`

```shell
$ make docker-test32-all
```

OR

```shell
$ make docker-test32 ARCH=<YOUR_ARCH>
```

#### 64-bit with `__uint128_t` support

requires Docker (with QEMU) and `docker-compose`

```shell
$ make docker-test-all
```

OR

```shell
$ make dokcer-test ARCH=<YOUR_ARCH>
```

## Dev and debug

```shell
$ make up ARCH=<YOUR_ARCH:arm64v8>
$ make shell
$ make down
```
