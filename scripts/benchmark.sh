#!/usr/bin/env bash
set -euo pipefail

# ---------- Config ----------
ITERATIONS=1000
THREADS=4
RUNS=3
PROFILE="profiles/MID1/MID1_Priest_Shadow.simc"
WORKDIR="${TMPDIR:-/tmp}/simc-bench"
SRCDIR="$(cd "$(dirname "$0")/.." && pwd)"

# ---------- Helpers ----------
die()  { echo "FATAL: $*" >&2; exit 1; }
info() { echo "==> $*" >&2; }

best_of() {
  printf '%s\n' "$@" | sort -g | head -1
}

bench_variant() {
  local binary="$1" label="$2"
  local best=999999 t
  for i in $(seq 1 "$RUNS"); do
    info "  run $i/$RUNS"
    t=$( { /usr/bin/time -p "$binary" \
        "$SRCDIR/$PROFILE" \
        iterations="$ITERATIONS" \
        threads="$THREADS" \
        output=/dev/null \
        html=/dev/null ; } 2>&1 | awk '/^real/ {print $2}' )
    [[ -n "$t" ]] || die "failed to capture time for $label run $i"
    best=$(best_of "$best" "$t")
  done
  echo "$best"
}

cmake_build() {
  local builddir="$1"; shift
  cmake -S "$SRCDIR" -B "$builddir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_GUI=OFF \
    -DBUILD_TESTING=OFF \
    "$@"
  cmake --build "$builddir" --target simc -j "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
}

# ---------- Pre-checks ----------
command -v cmake >/dev/null || die "cmake not found"

# llvm-profdata: try PATH first, then xcrun (macOS Xcode toolchain)
if command -v llvm-profdata >/dev/null 2>&1; then
  LLVM_PROFDATA="llvm-profdata"
elif xcrun -f llvm-profdata >/dev/null 2>&1; then
  LLVM_PROFDATA="$(xcrun -f llvm-profdata)"
else
  die "llvm-profdata not found (install llvm toolchain)"
fi
[[ -f "$SRCDIR/CMakeLists.txt" ]] || die "cannot find $SRCDIR/CMakeLists.txt"
[[ -f "$SRCDIR/$PROFILE" ]]       || die "cannot find $SRCDIR/$PROFILE"

rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"

# ---------- 1. Baseline ----------
info "Building baseline (Release, no extras)"
cmake_build "$WORKDIR/build-baseline"
info "Benchmarking baseline"
TIME_BASELINE=$(bench_variant "$WORKDIR/build-baseline/simc" "baseline")
info "  best: ${TIME_BASELINE}s"

# ---------- 2. LTO + march=native ----------
info "Building LTO + march=native"
cmake_build "$WORKDIR/build-lto" -DSC_LTO=ON -DSC_MARCH_NATIVE=ON
info "Benchmarking LTO + march=native"
TIME_LTO=$(bench_variant "$WORKDIR/build-lto/simc" "lto+march")
info "  best: ${TIME_LTO}s"

# ---------- 3. PGO ----------
info "Building PGO instrumented"
cmake_build "$WORKDIR/build-pgo-gen" -DSC_PGO_GENERATE=ON

info "Collecting PGO profile data"
LLVM_PROFILE_FILE="$WORKDIR/pgo-%p.profraw" \
  "$WORKDIR/build-pgo-gen/simc" \
  "$SRCDIR/$PROFILE" \
  iterations="$ITERATIONS" \
  threads="$THREADS" \
  output=/dev/null \
  html=/dev/null

info "Merging profile data"
PROFDATA="$WORKDIR/merged.profdata"
"$LLVM_PROFDATA" merge -output="$PROFDATA" "$WORKDIR"/pgo-*.profraw

info "Building PGO optimized (+ LTO + march=native)"
cmake_build "$WORKDIR/build-pgo-use" \
  -DSC_LTO=ON \
  -DSC_MARCH_NATIVE=ON \
  -DSC_PGO_USE="$PROFDATA"

info "Benchmarking PGO optimized"
TIME_PGO=$(bench_variant "$WORKDIR/build-pgo-use/simc" "pgo")
info "  best: ${TIME_PGO}s"

# ---------- Results ----------
echo ""
echo "=============================================="
echo " SimC build benchmark  (best of $RUNS runs)"
echo " iterations=$ITERATIONS  threads=$THREADS"
echo "=============================================="
printf "%-28s %8s %8s\n" "Variant" "Time(s)" "Speedup"
echo "----------------------------------------------"

for label_key in "Release (baseline):$TIME_BASELINE" "LTO + march=native:$TIME_LTO" "PGO + LTO + march=native:$TIME_PGO"; do
  label="${label_key%%:*}"
  t="${label_key#*:}"
  speedup=$(awk "BEGIN {printf \"%.2fx\", $TIME_BASELINE / $t}")
  printf "%-28s %8s %8s\n" "$label" "$t" "$speedup"
done
echo "=============================================="
echo "Build artifacts in: $WORKDIR"
