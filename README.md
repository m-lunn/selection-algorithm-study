# Selection algorithm study

This project compares selection algorithms for finding the element at a
zero-based rank `k` without fully sorting the input. It measures wall-clock
time and comparator calls on integer, string, duplicate-heavy, structured, and
McIlroy adversarial inputs.

## Algorithms

The benchmark uses the canonical implementations under `algorithms/`:

- `qs_random`: randomized quickselect with a seeded random pivot.
- `bfprt`: median of medians, with configurable group size.
- `floyd_rivest`: Floyd-Rivest selection with a configurable sampling threshold.
- `introselect`: randomized quickselect with a BFPRT fallback.
- `std_nth`: `std::nth_element` baseline.
- `std_sort`: full-sort baseline.

The implemented custom algorithms share:

- An insertion-sort cutoff of 16 by default.
- The Hoare partition and narrowing helpers in `algorithms/partition.hpp`.
- The insertion-sort helper in `algorithms/common.hpp`.

The `std_nth` and `std_sort` baselines use the standard library's own
implementation details and are not subject to the custom cutoff.

## Repository layout

| Path | Contents |
|---|---|
| `algorithms/` | Canonical templated algorithms and standalone `.cpp` wrappers. |
| `generators/` | Input generators, McIlroy adversary generator, string conversion, and generator sampler. |
| `benchmarking/` | Benchmark harness, algorithm adapters, correctness tests, plotting script, and experiment scripts. |
| `results/` | Generated CSV results, metadata, plots, and cached adversaries. |

## Requirements

- C++17 compiler: `g++` or `clang++`
- `make`
- Python 3
- For plots: `numpy`, `pandas`, and `matplotlib`

## Build and correctness tests

Run these commands from the repository root:

```bash
make
make test
```

`make test` runs the algorithm and generator tests with AddressSanitizer and
UndefinedBehaviorSanitizer. The checks cover:

- Correct value at `a[k]`.
- Correct partition around `a[k]`.
- Preservation of the input as a permutation.
- Sorted, reverse-sorted, random, nearly sorted, organ-pipe, sawtooth,
  all-equal, and few-distinct inputs.
- Integer and shared-prefix string inputs.
- Fixed-seed McIlroy adversary replay.
- Introselect's BFPRT fallback.

Expected output ends with messages like:

```text
all algorithm tests passed (...)
all generator tests passed
```

## Inspect generator samples

Print reproducible examples from every ordinary generator, strings, and a
known-seed McIlroy adversary:

```bash
./generators/show_samples.sh
```

The sampler uses fixed seeds and bounded previews, so it is safe to run before
large experiments.

## Run a small benchmark

Start with a small smoke test:

```bash
./build/bench \
  --families random,sorted,reverse,all_equal,organ_pipe \
  --sizes 128,1024 \
  --ranks min,median,max \
  --algos qs_random,bfprt,floyd_rivest,introselect,std_nth \
  --reps 3 \
  --seeds 2 \
  --out results/smoke.csv
```

The harness validates the first timed and counted run for every benchmark cell.
It aborts if an algorithm returns the wrong selected value or partition.

## Run the experiment scripts

The pilot script runs a smaller sanity sweep and creates plots:

```bash
benchmarking/run_pilot.sh
```

The full experiment script supports these phases:

```bash
benchmarking/run_all.sh main
benchmarking/run_all.sh adv
benchmarking/run_all.sh strings
benchmarking/run_all.sh params
benchmarking/run_all.sh all
```

The phases write CSV files under `results/`:

```text
results/main.csv
results/adversary.csv
results/strings.csv
results/params_bfprt.csv
results/params_fr.csv
results/params_intro.csv
```

They also write `.meta.txt` files containing the environment and command line,
plots under `results/plots_<phase>/`, and cached adversaries under
`results/adversaries/`.

The full sweep can take a long time and use substantial memory. Adjust the
size lists in `benchmarking/run_all.sh` for the machine being used.

The plot directory contains PNG charts and summary CSV files, including:

```text
time_vs_n.png
cmps_vs_n.png
time_vs_cmps.png
rank_sweep.png
summary_time.csv
summary_cmps.csv
```

## Understanding benchmark rows

Each CSV row contains:

```text
algo,type,family,n,seed,prefix,rank,k,rep,time_ns,comparisons,fallbacks,input_hash
```

- `rank` is a named target position such as `min`, `p25`, `median`, `p75`, or
  `max`; `k` is its zero-based index.
- A random family means the input order is shuffled. It does not mean `k` is
  randomly selected.
- `time_ns` measures only the selection call; copying the input is outside the
  timer.
- `comparisons` comes from a separate counted run and does not add comparator
  overhead to the timing.
- `fallbacks` records BFPRT fallback activations for introselect.
- `input_hash` lets you verify that algorithms received the same input instance
  for each family, size, seed, prefix, and rank.

For trustworthy timing comparisons, use multiple repetitions and report
medians and interquartile ranges rather than individual timings.

## Known-seed McIlroy adversaries

McIlroy adversaries can target a known random sequence:

```bash
./build/bench \
  --seed 12345 \
  --families mcilroy:qs_random,mcilroy:introselect \
  --sizes 128,1024 \
  --ranks median \
  --algos qs_random,introselect,bfprt,std_nth \
  --reps 1 \
  --out results/mcilroy-smoke.csv
```

The adversary is specific to the target algorithm, parameters, size, rank, and
seed. Changing the seed generally invalidates the attack. The seed is included
in the cached adversary filename.

## Adding or changing an algorithm

1. Add or update the canonical implementation under `algorithms/`.
2. Keep the selection contract: `a[k]` must contain the selected value, and the
   array must be partitioned around it.
3. Add the adapter and command-line name in `benchmarking/`.
4. Add representative correctness cases to
   `benchmarking/test_algorithms.cpp`.
5. Run `make test` before benchmarking.

## Methodology notes

- Every algorithm receives a fresh copy of the same input instance.
- Randomized algorithms use explicit, reproducible seeds.
- Correctness checks run before interpreting performance data.
- Custom algorithms use the same cutoff and shared Hoare partition, while
  library baselines remain separate.
- Comparison counts include comparisons in insertion sort and BFPRT group
  sorting; swaps and moves are not counted.
- Long string prefixes can use substantial memory; reduce `n` when using
  prefixes such as 256 or 1024.