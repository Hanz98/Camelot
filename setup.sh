conan profile detect
conan install . --output-folder=build --build=missing

cmake -S . -B build