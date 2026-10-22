# Selection benchmark: worst-case-linear vs. fast-in-practice

Benchmarking suite for k-th smallest element algorithms: quickselect (random and
median-of-3 pivot), median of medians (BFPRT), Floyd-Rivest, introselect, and
`std::nth_element`, on integer and expensive-comparison (string) inputs, including
McIlroy-style adversarial inputs. Every run records wall-clock time **and**
comparison counts.

## Layout

| Path | What |
|---|---|
| `algorithms/` | One header per algorithm (`quickselect.hpp`, `bfprt.hpp`, `floyd_rivest.hpp`, `introselect.hpp`), shared primitives (`common.hpp`), and `registry.hpp`, which turns a spec string such as `bfprt:g=3:c=16` into a callable. Primary author is named in each header. |
| `generators/` | Input generators (`generators.hpp`), McIlroy adversary (`mcilroy.hpp`), shared-prefix strings (`string_inputs.hpp`). |
| `bench/` | `bench.cpp` (harness CLI) and `counting.hpp` (comparison-counting comparator). |
| `tests/` | `test_algorithms.cpp`: correctness of every algorithm configuration. |
| `scripts/` | `run_pilot.sh`, `run_all.sh` (all experiments), `plot.py`. |
| `results/` | CSVs (+ `.meta.txt` with CPU/compiler/flags/command), plots, cached adversary arrays. |

## Quick start

    make                    # builds build/bench (override: make BUILD=/tmp/b if the repo is on a noexec mount)
    make test               # correctness tests under ASan+UBSan -- run before benchmarking
    scripts/run_pilot.sh    # small sanity sweep + plots in results/pilot_plots
    scripts/run_all.sh      # every experiment (main | adv | strings | params | all)

Requirements: g++ or clang++ with C++17; Python 3 with pandas and matplotlib for plotting.
For stable timings: `PIN=2 scripts/run_all.sh` (pins to core 2 on Linux), close other
programs, and note your CPU governor/turbo setting in the report.

Examples:

    build/bench --families random,organ_pipe --sizes 1e5,1e6 --ranks min,median,max \
                --algos bfprt:g=3,bfprt:g=5,floyd_rivest,std_nth --reps 11 --out results/x.csv
    build/bench --type string --prefixes 0,64,1024 --sizes 1e5 --families random \
                --algos qs_m3,bfprt,floyd_rivest --out results/s.csv
    build/bench --families mcilroy:qs_m3 --sizes 1e4 --ranks median --algos qs_m3,bfprt,std_nth

`build/bench --help` lists all options.

## CSV schema (one row per algorithm x instance x rank x repetition)

`algo, type, family, n, seed, prefix, rank, k, rep, time_ns, comparisons, fallbacks, input_hash`

* `time_ns` / `comparisons` are `-1` if that measurement was disabled.
* `input_hash` is a hash of the exact input; rows for different algorithms on the
  same instance have the same hash (evidence of identical inputs).
* `fallbacks` = times introselect abandoned quickselect for BFPRT (0 for other algorithms).
* Deterministic families (sorted, reverse, all_equal, organ_pipe, sawtooth, mcilroy)
  use a single instance (`seed` = base seed); random families use `--seeds` instances.

## Methodology built into the harness

* **Same instance for every algorithm.** The input is generated once per
  (family, n, seed, prefix, rank) and copied fresh into a work buffer before each run.
* **Timing** measures only the select call (`steady_clock`); the copy is outside the timer.
  One warm-up run first (`--warmup`). If the warm-up exceeds `--slow-ms` (default 3000 ms)
  the cell gets a single repetition, which keeps quadratic cases from stalling a sweep.
* **Counting is a separate run**, never inside the timed run, using the same input copy
  and RNG seed as that repetition's timed run (so randomized algorithms do the same work).
* **Correctness check** on the first repetition of every cell: `a[k]` equals the true k-th
  element and the array is partitioned around it. A failure aborts the run.
* **Randomized algorithms** (`qs_random`) are re-seeded per repetition; report the
  distribution over reps/seeds, not one run.
* **Environment** (CPU model, compiler version, flags, command line) is written to `<csv>.meta.txt`.

## Design decisions to describe in your report

* All algorithms share one Hoare/Wirth partition and one insertion-sort cutoff (16 by default),
  so differences come from pivot strategy, not partition quality. See `algorithms/common.hpp`.
  `std::nth_element` is the exception: it is libstdc++'s own code (cutoff 3, heap-select fallback).
* Floyd-Rivest follows the common Wirth-style variant (window-select, then partition around `a[k]`).
* Introselect here falls back to BFPRT; libstdc++'s falls back to heap-select. Treat `std_nth`
  and `introselect` as different algorithms.
* The McIlroy adversary attacks one *deterministic* algorithm with specific parameters, `n` and `k`.
  The frozen array is stored in `results/adversaries/` and replayed against all algorithms.
  Attacking `min` (k=0) with qs_m3 did not hurt it in the pilot, so sweep ranks.
* Comparison counts include comparisons inside insertion sort and the group sorts of BFPRT.
  Element moves and swaps are **not** counted.

## Adding an algorithm (e.g. arXiv:1606.00484)

1. Write `algorithms/<name>.hpp` with `template <class T, class Less> void name(T* a, size_t n, size_t k, Less less)`
   (must leave `a[k]` correct and the array partitioned around it). Use only `less(x, y)` to compare.
2. In `registry.hpp` add a `Kind`, a case in `parse_algo`, and a case in `Algo::operator()`.
3. Add its spec to the test list in `tests/test_algorithms.cpp`, run `make test`, then use the spec in `--algos`.

## Known limitations

* At small n (<= ~10^4) one run takes microseconds; use many repetitions and read the IQR bands.
* String inputs use `n * (prefix + digits)` bytes per copy (plus a work copy): cap n for long prefixes.
* The adversary is generated by running the quadratic algorithm, so `--max-adv-n` (default 2e5) limits it.
* Cache/branch-miss counters are not collected by the harness; use `perf stat` around a single
  `bench` invocation with `--no-count` if you want them.
