# Installs dependencies with Conan and configures/builds the project with CMake.
# Run from a "Developer PowerShell for VS 2022" so that cl.exe is on PATH
# (`pip install conan ninja` once beforehand).
#
# Knobs (environment variables), mirroring scripts/linux/setup.sh:
#   BUILD_TYPE     Debug (default) or Release
#   BUILD_TESTS    ON (default) / OFF   -> CMake BUILD_TESTING
#   RUN_TESTS      1 to run the test suite after the build
#   USE_MOCK_ICD   1 to run tests against the vendored mock Vulkan ICD
#                  (no GPU needed; default: 0)
#   CONAN_ARGS     extra arguments appended to `conan install`, e.g.
#                  '--build=implot/*' to rebuild a stale implot
#
# Layout (from conanfile.py cmake_layout):
#   build\<BUILD_TYPE>\generators\conan_toolchain.cmake
$ErrorActionPreference = "Stop"

$rootDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $rootDir

$buildType  = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { "Debug" }
$buildTests = if ($env:BUILD_TESTS) { $env:BUILD_TESTS } else { "ON" }
$mockIcd    = if ($env:USE_MOCK_ICD -eq "1") { "ON" } else { "OFF" }
$conanArgs  = if ($env:CONAN_ARGS) { $env:CONAN_ARGS -split " " } else { @() }
$profile    = "profiles/Camelot-Win"
$buildDir   = "build/$buildType"
$toolchain  = "$buildDir/generators/conan_toolchain.cmake"

Write-Host "Profile:    $profile"
Write-Host "Build type: $buildType"
Write-Host "Build dir:  $buildDir"
Write-Host "Mock ICD:   $mockIcd"

# No Vulkan SDK needed: the loader, the headers and glslang come from Conan.
# Build tools (glslang, nasm, ...) are always Release, whatever BUILD_TYPE is.
conan install . `
    --profile:host=$profile `
    --profile:build=$profile `
    -s build_type=$buildType `
    -s:b build_type=Release `
    --build=missing `
    @conanArgs
if ($LASTEXITCODE -ne 0) { throw "conan install failed" }
if (-not (Test-Path $toolchain)) { throw "Conan toolchain not found at $toolchain" }

cmake -S . -B $buildDir -G Ninja `
    -D CMAKE_TOOLCHAIN_FILE="$toolchain" `
    -D CMAKE_BUILD_TYPE=$buildType `
    -D CMAKE_EXPORT_COMPILE_COMMANDS=ON `
    -D BUILD_TESTING=$buildTests `
    -D CAMELOT_TESTS_USE_MOCK_ICD=$mockIcd
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

cmake --build $buildDir --config $buildType
if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }

if ($env:RUN_TESTS -eq "1") {
    # Single-config generators (Ninja) put it in bin/, multi-config ones
    # (Visual Studio) in bin/<config>/.
    $testExe = Join-Path $buildDir "bin/Test_Main.exe"
    if (-not (Test-Path $testExe)) { $testExe = Join-Path $buildDir "bin/$buildType/Test_Main.exe" }
    # vulkan-1.dll (and any other shared dependency) lives in the Conan cache,
    # so the tests run inside Conan's run environment. conanrun.bat is a batch
    # file, hence the detour through cmd: `--%` hands the rest of the line to
    # cmd verbatim and cmd expands the %...% variables itself.
    $env:CAMELOT_CONAN_RUN = (Resolve-Path (Join-Path $buildDir "generators/conanrun.bat")).Path
    $env:CAMELOT_TEST_EXE  = (Resolve-Path $testExe).Path
    cmd /c --% call "%CAMELOT_CONAN_RUN%" && "%CAMELOT_TEST_EXE%"
    if ($LASTEXITCODE -ne 0) { throw "tests failed" }
}

Write-Host "Done. Binaries are in $buildDir\bin"
