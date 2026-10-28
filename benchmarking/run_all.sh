#!/usr/bin/env bash
# Full experiment suite. Usage:  scripts/run_all.sh [main|adv|strings|params|all]
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
DET="bfprt,floyd_rivest,introselect,std_nth"
ALGOS="qs_random,$DET"
phase=${1:-all}

main() {   # 1. main sweep: every algorithm x every input family x several ranks
  "$BUILD/bench" $PINFLAG --reps "$REPS" --seeds "$SEEDS" --out results/main.csv \
    --families random,sorted,reverse,all_equal,organ_pipe,few_distinct:2,few_distinct:100,sawtooth:100,nearly_sorted:0.01 \
    --sizes "$SIZES" --ranks min,p25,median,p75,max --algos "$ALGOS"
  python3 benchmarking/plot.py results/main.csv --out results/plots_main
}
adv() {    # 2. McIlroy adversary: attack each deterministic algorithm, replay against all
  fams=""; for a in ${DET//,/ }; do fams="$fams,mcilroy:$a"; done; fams=${fams#,}
  "$BUILD/bench" $PINFLAG --reps "$REPS" --out results/adversary.csv \
    --families "$fams" --sizes 1e3,3e3,1e4,3e4,1e5 --ranks min,median,max \
    --algos "$ALGOS" --slow-ms 2000
  python3 benchmarking/plot.py results/adversary.csv --out results/plots_adv
}
strings() { # 3. expensive comparisons: shared-prefix strings
  "$BUILD/bench" $PINFLAG --type string --reps "$REPS" --seeds 2 --out results/strings.csv \
    --families random,sorted,organ_pipe,few_distinct:100 --sizes 1e4,1e5 --ranks median \
    --prefixes 0,16,64,256,1024 --algos "$ALGOS"
  python3 benchmarking/plot.py results/strings.csv --type string --out results/plots_strings
}
params() { # 4. parameter studies
  "$BUILD/bench" $PINFLAG --reps "$REPS" --seeds "$SEEDS" --out results/params_bfprt.csv \
    --families random,all_equal --sizes 1e4,1e5,1e6,1e7 --ranks median \
    --algos bfprt:g=3,bfprt:g=5,bfprt:g=7,bfprt:g=9,bfprt:g=11,bfprt:g=5:c=1,bfprt:g=5:c=4,bfprt:g=5:c=8,bfprt:g=5:c=32,bfprt:g=5:c=64
  python3 benchmarking/plot.py results/params_bfprt.csv --out results/plots_params --tag bfprt
  "$BUILD/bench" $PINFLAG --reps "$REPS" --seeds "$SEEDS" --out results/params_fr.csv \
    --families random,sorted --sizes 1e4,1e5,1e6,1e7 --ranks min,median \
    --algos floyd_rivest:thr=100,floyd_rivest:thr=300,floyd_rivest:thr=600,floyd_rivest:thr=1000,floyd_rivest:thr=3000,floyd_rivest:thr=1e9
  python3 benchmarking/plot.py results/params_fr.csv --out results/plots_params --tag fr
  "$BUILD/bench" $PINFLAG --reps "$REPS" --seeds "$SEEDS" --out results/params_intro.csv \
    --families random,organ_pipe,mcilroy:bfprt --sizes 1e3,1e4,1e5 --ranks median \
    --algos introselect:d=0.5,introselect:d=1,introselect:d=2,introselect:d=4,introselect:d=8,qs_random --slow-ms 2000
  python3 benchmarking/plot.py results/params_intro.csv --out results/plots_params --tag intro
}

case "$phase" in
  main) main ;; adv) adv ;; strings) strings ;; params) params ;;
  all) main; adv; strings; params ;;
  *) echo "usage: $0 [main|adv|strings|params|all]"; exit 1 ;;
esac
