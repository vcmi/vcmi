# RMG Benchmark

`vcmi-rmg-bench` is a headless command-line benchmark for random map generation.
It is meant for performance comparisons between commits, scheduler modes, and
thread counts.

## Build

Use the dedicated benchmark preset:

```sh
cmake --preset linux-gcc-bench
cmake --build --preset linux-gcc-bench --target vcmi-rmg-bench
```

Binary location:

```text
out/build/linux-gcc-bench/bin/vcmi-rmg-bench
```

## Defaults

The tool defaults to the heavy scenario discussed in PR feedback:

- `--width 504`
- `--height 504`
- `--levels 4`
- `--expected-zones 200`

This scenario requires compatible templates in loaded data/mods. If none match,
the benchmark now exits with a clear error instead of crashing.

If template selection cannot enforce exactly 200 zones, the benchmark prints the
actual zone count for traceability.

## Usage

List available templates:

```sh
out/build/linux-gcc-bench/bin/vcmi-rmg-bench --list-templates
```

Run with defaults:

```sh
out/build/linux-gcc-bench/bin/vcmi-rmg-bench
```

Run with explicit template and output CSV:

```sh
out/build/linux-gcc-bench/bin/vcmi-rmg-bench \
  --template-id vcmi:Clash\ of\ Dragons \
  --threads 8 \
  --scheduler parallel \
  --warmup 2 \
  --runs 20 \
  --output-csv /tmp/rmg-bench.csv
```

## Key flags

- `--template-id <id>` or `--template-path <json>`
- `--width`, `--height`, `--levels`
- `--players`, `--human-players`, `--comp-only-players`
- `--water random|none|normal|islands`
- `--monsters random|weak|normal|strong`
- `--seed`, `--seed-step`, `--timestamp`
- `--scheduler single|parallel`
- `--threads`
- `--warmup`, `--runs`
- `--expected-zones`
- `--output-csv <path>`
