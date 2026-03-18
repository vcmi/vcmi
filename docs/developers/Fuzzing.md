# Fuzzing

This document describes how to build and run VCMI fuzz targets locally.

The fuzzing build is currently intended for Clang with libFuzzer and sanitizers.

## Build with CMake presets

Use the dedicated configure and build presets:

```sh
cmake --preset linux-clang-fuzz
cmake --build --preset linux-clang-fuzz
```

Fuzzer binaries are written to `out/build/linux-clang-fuzz/bin/`.

## Nix shell for fuzzing

If you use Nix, enter the fuzzing shell to get Clang + compiler-rt:

```sh
nix develop ./nix#fuzzing
```

The default shell still works for regular builds and tests.

## Running fuzzers

Each target is a standalone libFuzzer executable. A typical invocation is:

```sh
out/build/linux-clang-fuzz/bin/<fuzzer-name> \
  fuzz/corpus/<fuzzer-name> \
  -max_total_time=120
```

Use `-runs=<N>` for deterministic smoke runs or `-max_total_time=<seconds>` for
time-bounded fuzzing.

RMG fuzzers currently available are:

```text
vcmi-fuzz-rmg-repro
vcmi-fuzz-rmg-validity
```
