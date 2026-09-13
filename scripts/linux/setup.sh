#!/usr/bin/env bash
# Installs dependencies with Conan and builds the project with CMake.
#
# Knobs (environment variables):
#   BUILD_TYPE     Release (default) or Debug
#   HOST_PROFILE   Conan host profile   (default: profiles/Camelot-Linux)
#   BUILD_PROFILE  Conan build profile  (default: same as HOST_PROFILE)
#   BUILD_TESTS    ON (default) / OFF   -> CMake BUILD_TESTING
#   RUN_TESTS      1 to run ctest after the build (default: 0)
#   USE_MOCK_ICD   1 to run tests against the vendored mock Vulkan ICD
#                  (no GPU needed; default: 0)
#   CONAN_ARGS     extra arguments appended to `conan install`, e.g.
#                  "-s compiler.version=13"
#
# Layout (from conanfile.py cmake_layout):
#   build/<BUILD_TYPE>/generators/conan_toolchain.cmake
#   build/<BUILD_TYPE>/compile_commands.json
set -euo pipefail

scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rootDir="$(cd "$scriptDir/../.." && pwd)"
cd "$rootDir"

BUILD_TYPE="${BUILD_TYPE:-Release}"
HOST_PROFILE="${HOST_PROFILE:-profiles/Camelot-Linux}"
BUILD_PROFILE="${BUILD_PROFILE:-$HOST_PROFILE}"
BUILD_TESTS="${BUILD_TESTS:-ON}"
RUN_TESTS="${RUN_TESTS:-0}"
USE_MOCK_ICD="${USE_MOCK_ICD:-0}"
CONAN_ARGS="${CONAN_ARGS:-}"

BUILD_DIR="build/${BUILD_TYPE}"
TOOLCHAIN="${BUILD_DIR}/generators/conan_toolchain.cmake"

echo "Host profile:   $HOST_PROFILE"
echo "Build profile:  $BUILD_PROFILE"
echo "Build type:     $BUILD_TYPE"
echo "Build dir:      $BUILD_DIR"

[[ -f "$HOST_PROFILE"  ]] || { echo "Host profile not found: $HOST_PROFILE" >&2; exit 1; }
[[ -f "$BUILD_PROFILE" ]] || { echo "Build profile not found: $BUILD_PROFILE" >&2; exit 1; }

echo "== Conan install"
# shellcheck disable=SC2086  # CONAN_ARGS is intentionally word-split
conan install . \
  --profile:host="$HOST_PROFILE" \
  --profile:build="$BUILD_PROFILE" \
  -s build_type="$BUILD_TYPE" \
  --build=missing \
  $CONAN_ARGS

[[ -f "$TOOLCHAIN" ]] || { echo "Conan toolchain not found at $TOOLCHAIN" >&2; exit 1; }

echo "== CMake configure"
MOCK_ICD_FLAG="OFF"
[[ "$USE_MOCK_ICD" == "1" ]] && MOCK_ICD_FLAG="ON"
cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DBUILD_TESTING="$BUILD_TESTS" \
  -DCAMELOT_TESTS_USE_MOCK_ICD="$MOCK_ICD_FLAG"

echo "== CMake build"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE"

if [[ "$RUN_TESTS" == "1" ]]; then
  echo "== Tests"
  ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

echo "Done. Binaries are in $BUILD_DIR/bin"
