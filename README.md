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
build/Release/bin/Test_Main
```

### Windows

Open a *Developer PowerShell for VS 2022*, `pip install conan ninja` once, and run:

```powershell
scripts\windows\setup.ps1                                        # Debug build
$env:BUILD_TYPE = "Release"; scripts\windows\setup.ps1           # Release build
$env:RUN_TESTS = 1; $env:USE_MOCK_ICD = 1; scripts\windows\setup.ps1   # build + run tests without a GPU
```

It uses `profiles/Camelot-Win` (MSVC 19.4x, Ninja) and the same
`build/<BuildType>` layout; the knobs are the same as on Linux (`BUILD_TYPE`,
`BUILD_TESTS`, `RUN_TESTS`, `USE_MOCK_ICD`, `CONAN_ARGS`). No Vulkan SDK is
needed: the loader, the headers and glslang come from Conan, and packages
without a prebuilt binary are built from source (together with their tool
requirements, e.g. `nasm`), so the first install takes a while. Because
`vulkan-1.dll` lives in the Conan cache, the script runs `Test_Main.exe`
inside Conan's run environment (`build\<BuildType>\generators\conanrun.bat`);
do the same when running it by hand. With the mock ICD, the manifest and
`VkICD_mock_icd.dll` sit side by side in `build\<BuildType>\bin`.

If a link fails with `undefined reference to ImGui::GetForegroundDrawList()`,
an implot binary built against a non-docking imgui is in your Conan cache;
rebuild it with `CONAN_ARGS='--build=implot/*'` (see the CI section).

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

Tests are built when `BUILD_TESTING` is ON (the default) into one GoogleTest
binary. Run it directly:

```sh
build/Release/bin/Test_Main                      # whole suite
build/Release/bin/Test_Main --gtest_filter='Camera*'
build/Release/bin/Test_Main --gtest_output=xml:report.xml
```

Configure with `-DCAMELOT_TESTS_USE_MOCK_ICD=ON` to run against the vendored
mock Vulkan ICD; the binary then points the Vulkan loader at it by itself
(this is what CI does: under `xvfb-run` on Linux, on the runner's desktop
session on Windows, so no GPU is required). `ctest` still works and runs the
same binary as a single test, which keeps IDE test explorers happy.

## Continuous integration

Every pull request runs (`.github/workflows/`):

| Job | Workflow | What it does |
|-----|----------|--------------|
| `Build and test` | `build.yaml` | Ubuntu, gcc, Conan + Ninja, `Test_Main` on the mock ICD under `xvfb-run` |
| `Build and test (Windows)` | `build.yaml` | `windows-latest`, `profiles/Camelot-Win` with `compiler.version` overridden to the runner's MSVC toolset (as the Linux job does for gcc), Conan + Ninja, `Test_Main.exe` on the mock ICD; no Vulkan SDK, no xvfb |
| `clang-tidy and cppcheck` | `build.yaml` | static analysis on the configured Linux build |
| `Pre-commit Checks` | `pre-commit.yaml` | the hooks of `.pre-commit-config.yaml` |

The Linux setup shared by the build and static-analysis jobs lives in
`.github/actions/setup-build/action.yaml`. The Windows job is a separate job,
not a matrix entry, so a Windows failure never masks a Linux one; build and
test logs are uploaded as artifacts when a job fails.

Both build jobs cache `~/.conan2/p`, keyed on `conanfile.py` and the platform
profile, with a fallback to any older cache of the same OS. That fallback is
why `conan install` runs with `--build="implot/*"`: implot's package id only
encodes imgui's minor version (`imgui/1.92.Z`), so an implot binary built
against the non-docking imgui its recipe pins is indistinguishable from one
built against our forced `imgui/1.92.5-docking` and would be reused with
`--build=missing`, failing the link with
`undefined reference to ImGui::GetForegroundDrawList()`. Rebuilding implot
takes seconds; every other package's id changes with what it was built for.

## Coding style

See [docs/CODING_STYLE.md](docs/CODING_STYLE.md): Google C++ style via the
committed `.clang-format`, `avalon` / `camelot` namespaces, project headers
included by their path from the repository root.

## Linting, formatting and static analysis

Two layers, both enforced on every pull request:

1. **pre-commit** (`.pre-commit-config.yaml`) – fast checks that need no
   build: whitespace and line endings, license headers, clang-format (`.clang-format`,
   pinned clang-format version), cpplint, cmake-lint
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
