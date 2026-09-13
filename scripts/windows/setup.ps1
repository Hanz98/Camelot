# Installs dependencies with Conan and configures/builds the project with CMake.
# Run from a "Developer PowerShell for VS 2022" so that cl.exe and Ninja are on PATH.
#
# Knobs (environment variables):
#   BUILD_TYPE   Debug (default) or Release
#   RUN_TESTS    1 to run ctest after the build
#
# Layout (from conanfile.py cmake_layout):
#   build\<BUILD_TYPE>\generators\conan_toolchain.cmake
$ErrorActionPreference = "Stop"

$rootDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $rootDir

$buildType = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { "Debug" }
$profile   = "profiles/Camelot-Win"
$buildDir  = "build/$buildType"
$toolchain = "$buildDir/generators/conan_toolchain.cmake"

Write-Host "Profile:    $profile"
Write-Host "Build type: $buildType"
Write-Host "Build dir:  $buildDir"

conan install . `
    --profile:host=$profile `
    --profile:build=$profile `
    -s build_type=$buildType `
    --build=missing
if ($LASTEXITCODE -ne 0) { throw "conan install failed" }
if (-not (Test-Path $toolchain)) { throw "Conan toolchain not found at $toolchain" }

cmake -S . -B $buildDir -G Ninja `
    -D CMAKE_TOOLCHAIN_FILE="$toolchain" `
    -D CMAKE_BUILD_TYPE=$buildType `
    -D CMAKE_EXPORT_COMPILE_COMMANDS=ON
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

cmake --build $buildDir --config $buildType
if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }

if ($env:RUN_TESTS -eq "1") {
    ctest --test-dir $buildDir --output-on-failure -C $buildType
    if ($LASTEXITCODE -ne 0) { throw "tests failed" }
}

Write-Host "Done. Binaries are in $buildDir\bin"
