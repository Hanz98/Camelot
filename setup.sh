#!/bin/bash

conan install . --output-folder=build --build=missing

cmake -S . -B build -D CMAKE_TOOLCHAIN_FILE='build/build/Release/generators/conan_toolchain.cmake' -DCMAKE_BUILD_TYPE=Release 
cmake --build build