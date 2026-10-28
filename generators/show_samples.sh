#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD=${BUILD:-build}
make "BUILD=$BUILD" "$BUILD/show_generators" >/dev/null
"$BUILD/show_generators"
