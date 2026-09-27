#!/usr/bin/env bash
# Runs clang-tidy and cppcheck over the project sources.
#
# Both tools read the compile database of an existing build, so run
# scripts/linux/setup.sh (or any configure with CMAKE_EXPORT_COMPILE_COMMANDS)
# first. clang-tidy is configured by .clang-tidy (and Tests/.clang-tidy),
# cppcheck by .cppcheck-suppressions. CI runs this script on every pull
# request; see the static-analysis job in .github/workflows/build.yaml.
#
# Usage: runStaticAnalysis.sh [--clang-tidy | --cppcheck] [file...]
#   With no file arguments every tracked .cpp file outside Tests/ext is
#   analysed; headers are covered through clang-tidy's HeaderFilterRegex.
#
# Knobs (environment variables):
#   BUILD_TYPE   Release (default) or Debug; selects build/<BUILD_TYPE>
#   CLANG_TIDY   clang-tidy executable (default: clang-tidy)
#   CPPCHECK     cppcheck executable  (default: cppcheck)
#   JOBS         parallel clang-tidy processes (default: nproc)
set -euo pipefail

scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rootDir="$(cd "$scriptDir/../.." && pwd)"
cd "$rootDir"

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="build/${BUILD_TYPE}"
CLANG_TIDY="${CLANG_TIDY:-clang-tidy}"
CPPCHECK="${CPPCHECK:-cppcheck}"
JOBS="${JOBS:-$(nproc)}"

run_tidy=1
run_cppcheck=1
files=()
for arg in "$@"; do
  case "$arg" in
    --clang-tidy) run_cppcheck=0 ;;
    --cppcheck) run_tidy=0 ;;
    -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
    *) files+=("$arg") ;;
  esac
done

if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
  echo "No compile database at $BUILD_DIR/compile_commands.json." >&2
  echo "Configure the project first, e.g. scripts/linux/setup.sh" >&2
  exit 1
fi

if [[ ${#files[@]} -eq 0 ]]; then
  mapfile -t files < <(git ls-files -- '*.cpp' ':!Tests/ext/**')
fi

failed=()

if (( run_tidy )); then
  echo "== clang-tidy ($("$CLANG_TIDY" --version | grep -oE 'version [0-9.]+'))"
  # compile_commands.json comes from the GCC build; clang cannot use GCC's
  # precompiled headers and would otherwise error on every translation unit.
  if ! printf '%s\0' "${files[@]}" \
      | xargs -0 -P "$JOBS" -n 4 "$CLANG_TIDY" -p "$BUILD_DIR" --quiet \
          --extra-arg=-Wno-ignored-gch; then
    failed+=(clang-tidy)
  fi
fi

if (( run_cppcheck )); then
  echo "== cppcheck ($("$CPPCHECK" --version))"
  # cppcheck 2.13 (Ubuntu 24.04) ignores -i for --project inputs, so the
  # third-party (Tests/ext, Conan packages such as the imgui backends) and
  # generated translation units are dropped from a filtered copy of the
  # compile database instead.
  filtered_db="$BUILD_DIR/compile_commands.cppcheck.json"
  python3 - "$BUILD_DIR/compile_commands.json" "$filtered_db" "$rootDir" <<'PY'
import json, sys
src, dst = sys.argv[1], sys.argv[2]
entries = json.load(open(src))
root = sys.argv[3]
keep = [e for e in entries
        if e["file"].startswith(root + "/")
        and "/Tests/ext/" not in e["file"] and "/build/" not in e["file"]]
json.dump(keep, open(dst, "w"), indent=1)
print(f"   {len(keep)} of {len(entries)} translation units")
PY
  if ! "$CPPCHECK" \
      --project="$filtered_db" \
      --enable=warning,performance,portability \
      --inconclusive \
      --std=c++20 \
      --suppressions-list=.cppcheck-suppressions \
      --library=googletest \
      --inline-suppr \
      -i "$rootDir/Tests/ext" \
      -i "$rootDir/build" \
      --error-exitcode=1 \
      --quiet \
      -j "$JOBS" \
      --template='{file}:{line}:{column}: {severity}: {message} [{id}]'; then
    failed+=(cppcheck)
  fi
fi

if [[ ${#failed[@]} -ne 0 ]]; then
  echo "Failed: ${failed[*]}" >&2
  exit 1
fi
echo "All analyzers passed."
