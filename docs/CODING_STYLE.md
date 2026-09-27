# Coding style

The codebase follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
with the deviations and project-specific rules listed here. Formatting is
enforced by clang-format (`.clang-format`), naming and layout by cpplint and
clang-tidy (see the README section on linting). When this document and a tool
disagree, fix the tool configuration rather than the code.

## Layout

| Module | Namespace | Purpose |
|---|---|---|
| `Avalon/` | `avalon` | Vulkan engine. Knows nothing about MCAP or the scene. |
| `Camelot/` | `camelot` | Application model: scene, playback, data ingestion. |
| `Excalibur/` | (global, C ABI) | Plugin entry point that hands out a `camelot::ICamelot`. |
| `Pendragon/` | (global) | The executable. |
| `Tests/` | namespace of the code under test | GoogleTest suites, one file per class. |

- One class per file. The file is named after the class: `SwapchainModel` lives
  in `SwapchainModel.h` / `SwapchainModel.cpp`.
- Headers use `.h`, sources `.cpp`. No `.hpp`.
- Every header has an include guard derived from its path, as cpplint expects:
  `AVALON_SRC_WINDOW_WINDOW_H_`.
- Everything except `main()` and the `extern "C"` plugin API lives in the
  module namespace. Anonymous namespaces hold file-local helpers.

## Includes

Project headers are quoted and written with the path from the repository
root; third-party and standard headers use angle brackets:

```cpp
#include "Avalon/src/window/Window.h"   // the header this file implements, first

#include <GLFW/glfw3.h>                 // third-party
#include <vulkan/vulkan.h>

#include <memory>                       // standard library
#include <string>

#include "Avalon/src/device/Device.h"   // other project headers
```

clang-format regroups and sorts the blocks automatically. Include what you
use; do not rely on `pch.h` for declarations in headers.

## Naming

| Entity | Style | Example |
|---|---|---|
| Types (class, struct, enum, alias) | UpperCamelCase | `RenderPass`, `ImageCreateInfo` |
| Functions and methods | lowerCamelCase | `drawFrame()`, `getExtent()` |
| Private / protected data members | `m_` + lowerCamelCase | `m_swapchain` |
| Public data members of plain structs | lowerCamelCase, no prefix | `info.mipLevels` |
| Local variables and parameters | lowerCamelCase | `imageIndex` |
| Compile-time constants | `k` + UpperCamelCase | `kFramesInFlight` |
| Macros | UPPER_SNAKE_CASE, project-prefixed | `VK_CHECK_RESULT` |
| Namespaces | lowercase | `avalon` |
| Enumerators | `k` + UpperCamelCase (scoped enums) | `Stage::kVertex` |

Interfaces (pure abstract classes) carry an `I` prefix: `IGlfWrapper`,
`ICamelot`.

## Classes

- Order sections `public:`, `protected:`, `private:`; within a section:
  types, constructors and destructor, methods, then data members.
- Objects that own a Vulkan handle are move-only: delete the copy
  operations, implement move construction and assignment with
  `std::exchange`, and release the handle in the destructor. Destructors and
  move operations are `noexcept` and must not throw; log and leak instead.
- Use default member initialisers for handles (`VkImage m_image{VK_NULL_HANDLE};`).
- Mark getters `[[nodiscard]]` and `const`.
- Pass `std::shared_ptr` by value only when the callee stores it; otherwise
  by `const&`.

## Vulkan

- Zero-initialise `Vk*CreateInfo` structs with `= {}` and set `sType` first.
- Wrap every call that returns `VkResult` in `VK_CHECK_RESULT()`, which logs
  and throws `std::runtime_error` on failure.
- Prefer `std::array` / `std::vector` plus `.data()` and `.size()` over C
  arrays and magic counts.
- Report failures with `spdlog::error()` and then throw; never only log.

## Comments

- `//` comments, full sentences. Explain why, not what.
- Every public class gets a short comment on its responsibility and
  lifetime; every non-obvious method a one-line comment above the
  declaration.
- Suppressions carry a reason: `// NOLINT(check): why`,
  `// cppcheck-suppress id ; why`.

## Tests

- One `*Test.cpp` per class under `Tests/<module>/src/<same path>`.
- Test names: `TEST(ClassTest, WhatItVerifies)`; fixtures
  `class ClassTest : public ::testing::Test`.
- Tests that need a GPU run on the mock ICD in CI; anything that must not run
  there checks `CAMELOT_TESTS_USE_MOCK_ICD`.
