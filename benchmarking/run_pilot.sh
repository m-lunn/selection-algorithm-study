#!/usr/bin/env bash
# Quick sanity sweep (a few minutes at most): catches harness bugs and helps pick n ranges.
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD=${BUILD:-build}
make BUILD="$BUILD" >/dev/null
"$BUILD/bench" \
  --families random,sorted,all_equal,organ_pipe,few_distinct:2,mcilroy:qs_m3 \
  --sizes 1e3,1e4,1e5 --ranks min,median,max \
  --algos qs_random,qs_m3,bfprt,floyd_rivest,introselect,std_nth \
  --reps 5 --seeds 2 --out results/pilot.csv
python3 scripts/plot.py results/pilot.csv --out results/pilot_plots
