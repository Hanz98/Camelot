#!/bin/bash
conan install . --output-folder=build --build=missing

mkdir -p build
cd build

cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
cmake --build .