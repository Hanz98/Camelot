if (test-path "build") {
    Remove-Item -Recurse -Force "build"
}
conan profile detect
conan install . --build=missing -s build_type=Debug

cmake `
    -S .  `
    -B build  `
    -G "Visual Studio 17 2022"  `
    -D CMAKE_TOOLCHAIN_FILE='build/generators/conan_toolchain.cmake'
