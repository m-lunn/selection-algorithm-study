#!/usr/bin/env bash
# Full experiment suite. Usage:  scripts/run_all.sh [main|adv|strings|all]
# Environment: BUILD=build  PIN=2 (core to pin to)  REPS=11  SEEDS=3
# Every phase writes its own CSV (+ .meta.txt) to results/ and its plots to results/plots_<phase>/.
# Edit the size lists below to fit your machine; large n x many reps takes a long time.
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD=${BUILD:-build}
REPS=${REPS:-11}
SEEDS=${SEEDS:-3}
PINFLAG=""; [ -n "${PIN:-}" ] && PINFLAG="--pin $PIN"
make BUILD="$BUILD" >/dev/null
make BUILD="$BUILD" test            # never benchmark an incorrect implementation

SIZES=1e3,3e3,1e4,3e4,1e5,3e5,1e6,3e6,1e7
ADV_TARGETS="qs_random,bfprt,floyd_rivest,introselect,std_nth"
ALGOS="qs_random,bfprt,floyd_rivest,introselect,std_nth"
phase=${1:-all}

main() {   # 1. main sweep: every algorithm x every input family x several ranks
  "$BUILD/bench" $PINFLAG --reps "$REPS" --seeds "$SEEDS" --out results/main.csv \
    --families random,sorted,reverse,all_equal,organ_pipe,few_distinct:2,few_distinct:100,sawtooth:100,nearly_sorted:0.01 \
    --sizes "$SIZES" --ranks min,p25,median,p75,max --algos "$ALGOS"
  python3 benchmarking/plot.py results/main.csv --out results/plots_main
}
adv() {    # 2. McIlroy adversary: attack each configured algorithm, replay against all
  fams=""; for a in ${ADV_TARGETS//,/ }; do fams="$fams,mcilroy:$a"; done; fams=${fams#,}
  "$BUILD/bench" $PINFLAG --reps "$REPS" --out results/adversary.csv \
    --families "$fams" --sizes 1e3,3e3,1e4,3e4,1e5 --ranks min,median,max \
    --algos "$ALGOS" --slow-ms 2000
  python3 benchmarking/plot.py results/adversary.csv --out results/plots_adv
}
strings() { # 3. expensive comparisons: shared-prefix strings
  "$BUILD/bench" $PINFLAG --type string --reps "$REPS" --seeds 2 --out results/strings.csv \
    --families random,sorted,reverse,all_equal,organ_pipe,few_distinct:2,few_distinct:100,sawtooth:100,nearly_sorted:0.01 \
    --sizes 1e4,1e5 --ranks median \
    --prefixes 0,16,64,256,1024 --algos "$ALGOS"
  python3 benchmarking/plot.py results/strings.csv --type string --out results/plots_strings
}
case "$phase" in
  main) main ;; adv) adv ;; strings) strings ;;
  all) main; adv; strings ;;
  *) echo "usage: $0 [main|adv|strings|all]"; exit 1 ;;
esac
