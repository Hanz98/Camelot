#!/bin/bash


scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

tmp="$(dirname "$scriptDir")"

rootDir="$(dirname "$tmp")"

cd $rootDir

echo Starting the build procedure!

conan profile detect -e --name Camelot

echo Profile detected!

conan install . --output-folder=build --build=missing 

echo Conan installed!

mkdir -p build
cd build

echo Starting the build procedure!

cmake .. -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build .
