# Camelot

![Build and Test status](https://github.com/Hanz98/Camelot/actions/workflows/build.yaml/badge.svg?branch=develop)

A C++20 Vulkan rendering application. Development happens on the `develop`
branch; `master` only receives merges from it.

## Modules

| Module      | Role                                                                 |
|-------------|----------------------------------------------------------------------|
| `Avalon`    | Vulkan engine layer: window (GLFW), instance/device (vk-bootstrap), surface, swapchain, images, VMA allocator, render loop, shaders |
| `Camelot`   | Application model that wraps Avalon behind the `ICamelot` interface   |
| `Pendragon` | Executable entry point                                               |
| `Tests`     | GoogleTest suite, with a vendored mock Vulkan ICD for GPU-less runs  |
| `Excalibur`, `Merlin`, `Nimue` | Placeholders, not built                            |

## Dependencies

All third-party libraries (GLFW, spdlog, vk-bootstrap, Vulkan headers/loader,
VulkanMemoryAllocator, GoogleTest) and the GLSL compiler (glslang, a Conan
`tool_requires`) come from Conan 2 (`conanfile.py`).
You need CMake ≥ 3.20, Ninja, a C++20 compiler and `pip install conan`.

Conan uses `cmake_layout`, so everything lands under `build/<BuildType>/`:
the toolchain file is `build/<BuildType>/generators/conan_toolchain.cmake`
and binaries end up in `build/<BuildType>/bin`.

## Build

### Linux

```sh
scripts/linux/setup.sh                  # Release build
BUILD_TYPE=Debug scripts/linux/setup.sh # Debug build
RUN_TESTS=1 USE_MOCK_ICD=1 scripts/linux/setup.sh   # build + run tests without a GPU
```

The script uses `profiles/Camelot-Linux` (gcc 14). If your gcc differs, pass
`CONAN_ARGS="-s compiler.version=<major>"`. GLFW needs the X11 development
packages; Conan installs them when run with
`CONAN_ARGS="-c tools.system.package_manager:mode=install"`.

Equivalent manual steps:

```sh
conan install . --profile:host=profiles/Camelot-Linux --profile:build=profiles/Camelot-Linux \
    -s build_type=Release --build=missing
cmake --preset release
cmake --build --preset release
ctest --preset release
```

### Windows

Open a *Developer PowerShell for VS 2022* and run:

```powershell
scripts\windows\setup.ps1              # Debug build (BUILD_TYPE=Release for Release)
```

It uses `profiles/Camelot-Win` (MSVC 19.4, Ninja) and the same `build/<BuildType>` layout.

## Shaders

GLSL sources live in `Avalon/shaders/` and are listed in
`Avalon/CMakeLists.txt` under `camelot_add_shaders()` (defined in
`cmake/CamelotShaders.cmake`). At build time each file is compiled with
`glslangValidator` to `build/<BuildType>/shaders/<name>.spv` and embedded into
the `Avalon_shaders` library; code looks a module up by its source file name:

```cpp
#include <Avalon/shaders/Registry.h>
#include <Avalon/src/shader/ShaderModule.h>

ShaderModule vert(device, avalon::shaders::get("triangle.vert"));
```

Editing one GLSL file rebuilds only that module. Adding a shader means adding
the file to the `SOURCES` list; a listed file that does not exist fails at
configure time.

## Tests

Tests are built when `BUILD_TESTING` is ON (the default). Run them with
`ctest --test-dir build/Release --output-on-failure`. Configure with
`-DCAMELOT_TESTS_USE_MOCK_ICD=ON` to run against the vendored mock Vulkan ICD
(this is what CI does, under `xvfb-run`, so no GPU or display is required).

## Linting, formatting and static analysis

Two layers, both enforced on every pull request:

1. **pre-commit** (`.pre-commit-config.yaml`) – fast checks that need no
   build: whitespace and line endings, license headers, clang-format (Google
   style, pinned clang-format version), cpplint, cmake-lint
   (`.cmake-format.yaml`), shellcheck, codespell, yamllint (`.yamllint.yaml`)
   and actionlint for the workflows. Install once with
   `pip install pre-commit && pre-commit install`; run everything with
   `pre-commit run --all-files`. CI job: `Pre-commit Checks`.
2. **Static analysis** – clang-tidy (`.clang-tidy`, with `Tests/.clang-tidy`
   relaxing a few checks for test code) and cppcheck
   (`.cppcheck-suppressions`). Both read `build/<BuildType>/compile_commands.json`,
   so configure the project first, then run
   `scripts/linux/runStaticAnalysis.sh` (`--clang-tidy` or `--cppcheck` to
   run one of them, file paths to limit the scope). Warnings are errors. CI
   job: `clang-tidy and cppcheck`, using the clang-tidy 22 wheel and
   cppcheck from apt.

Suppress a finding inline only with a reason, e.g.
`// NOLINT(check-name): why` or `// cppcheck-suppress id ; why`.
