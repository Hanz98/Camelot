#!/usr/bin/env bash
set -euo pipefail

# repo root
scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rootDir="$(cd "$scriptDir/../.." && pwd)"
cd "$rootDir"

# Configurable knobs (env vars override these)
BUILD_TYPE="${BUILD_TYPE:-Release}"
HOST_PROFILE="${HOST_PROFILE:-profiles/Camelot-Linux}"
BUILD_PROFILE="${BUILD_PROFILE:-$HOST_PROFILE}"

echo "Using host profile:  $HOST_PROFILE"
echo "Using build profile: $BUILD_PROFILE"
echo "Build type:          $BUILD_TYPE"

# Sanity checks
[[ -f "$HOST_PROFILE"  ]] || { echo "Host profile not found: $HOST_PROFILE" >&2; exit 1; }
[[ -f "$BUILD_PROFILE" ]] || { echo "Build profile not found: $BUILD_PROFILE" >&2; exit 1; }

echo "Starting the build procedure!"

# Generate Conan files into ./build using your local profiles
conan install . \
  --output-folder=build \
  --profile:host="$HOST_PROFILE" \
  --profile:build="$BUILD_PROFILE" \
  -s build_type="$BUILD_TYPE" \
  --build=missing

echo "Conan installed!"

# Configure CMake with the Conan toolchain
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "Starting the build procedure!"
cmake --build build --config "$BUILD_TYPE"
