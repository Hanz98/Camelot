#!/bin/bash


scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

tmp="$(dirname "$scriptDir")"

rootDir="$(dirname "$tmp")"

cd $rootDir

echo Starting the build procedure!

conan profile detect -e --name Camelot

conan install . --output-folder=build --build=missing 

mkdir -p build
cd build

echo Starting the build procedure!

cmake .. -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake
cmake --build .
