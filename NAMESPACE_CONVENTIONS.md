# Namespace Conventions

This document outlines the namespace conventions adopted in the Camelot codebase as part of the C++ Core Guidelines compliance refactoring.

## Overview

All classes in the Camelot codebase are now enclosed in proper C++ `namespace {}` blocks to improve code organization, avoid global namespace pollution, and ensure consistent, modern C++ practices.

## Namespace Structure

The project uses a simple, flat namespace structure based on the logical modules:

### Avalon Namespace
**Purpose:** Graphics and rendering engine components  
**Namespace:** `Avalon`

**Classes:**
- `Avalon::Window` - Window management
- `Avalon::WindowManager` - Window manager (placeholder)
- `Avalon::Device` - Vulkan device abstraction
- `Avalon::Instance` - Vulkan instance abstraction
- `Avalon::ValidationLayer` - Vulkan validation layer utilities
- `Avalon::Engine` - Main graphics engine

### Camelot Namespace
**Purpose:** Core engine components  
**Namespace:** `Camelot`

**Classes:**
- `Camelot::Interface` - Core engine interface
- `Camelot::Engine` - Main engine implementation

### Merlin Namespace  
**Purpose:** Merlin module functionality  
**Namespace:** `Merlin`

**Classes:**
- `Merlin::Engine` - Merlin engine implementation

### Nimue Namespace
**Purpose:** Nimue module functionality  
**Namespace:** `Nimue`

**Classes:**
- `Nimue::Engine` - Nimue engine implementation

## Class Naming Rules

### Prefixes and Suffixes Removed
- **No "I" prefix** for interfaces (e.g., `ICamelot` → `Camelot::Interface`)
- **No "Wrapper" suffix** (e.g., `ValidationLayerWrapper` → `Avalon::ValidationLayer`)
- **No "C" prefix** for classes
- **No "Impl" suffix** for implementations

### Descriptive Naming
- **Avoid single-word or vague names** - Use clear, descriptive compound names
- **Examples of improvements:**
  - `Avalon` → `Avalon::Engine`
  - `MainModel` → `Camelot::Engine` / `Nimue::Engine`
  - `Merlin` → `Merlin::Engine`

### Naming Style
- **Use CamelCase** for class names
- **No underscores** in class names
- **Clear and descriptive** names that indicate purpose

## Usage Examples

### Before (Global Namespace)
```cpp
// Old style - classes in global namespace
#include <window/Window.h>
#include <device/Device.h>

Window window;
Device device;
ValidationLayerWrapper validation;
```

### After (Namespaced)
```cpp
// New style - classes in proper namespaces
#include <Avalon/src/window/Window.h>
#include <Avalon/src/device/Device.h>

Avalon::Window window;
Avalon::Device device;
Avalon::ValidationLayer validation;
```

### Using Declarations (Optional)
```cpp
// For heavily used classes, you can use using declarations
using Avalon::Window;
using Avalon::Device;

Window window;      // Now refers to Avalon::Window
Device device;      // Now refers to Avalon::Device
```

### Namespace Aliases (For Long Names)
```cpp
// Create aliases for convenience if needed
namespace graphics = Avalon;

graphics::Window window;
graphics::Device device;
```

## Implementation Notes

### Header Files
All class declarations are wrapped in their respective namespace blocks:

```cpp
namespace Avalon {

class Window {
    // Class implementation
};

}  // namespace Avalon
```

### Source Files  
All class implementations are also wrapped in the same namespace:

```cpp
namespace Avalon {

void Window::init(int width, int height, const std::string& title) {
    // Implementation
}

}  // namespace Avalon
```

### Cross-Namespace Usage
When using classes from other namespaces, use fully qualified names:

```cpp
namespace Camelot {

class Engine : public Interface {
private:
    Avalon::Window m_window;  // Fully qualified name
    
public:
    void test();
};

}  // namespace Camelot
```

## Guidelines

1. **Keep namespaces flat** - Avoid unnecessary deep nesting
2. **Use module names** as namespace names for clear logical separation
3. **Always close namespaces** with descriptive comments
4. **Prefer full qualification** over using declarations in header files
5. **Use descriptive class names** that clearly indicate their purpose
6. **Follow CamelCase** naming convention consistently

## Migration Summary

| Old Class Name | New Namespaced Name | Rationale |
|----------------|---------------------|-----------|
| `Window` | `Avalon::Window` | Added namespace |
| `Device` | `Avalon::Device` | Added namespace |
| `Instance` | `Avalon::Instance` | Added namespace |
| `ValidationLayerWrapper` | `Avalon::ValidationLayer` | Added namespace + removed "Wrapper" suffix |
| `Avalon` | `Avalon::Engine` | Added namespace + more descriptive name |
| `ICamelot` | `Camelot::Interface` | Added namespace + removed "I" prefix |
| `MainModel` (Camelot) | `Camelot::Engine` | Added namespace + more descriptive name |
| `Merlin` | `Merlin::Engine` | Added namespace + more descriptive name |
| `MainModel` (Nimue) | `Nimue::Engine` | Added namespace + more descriptive name |

This refactoring ensures the Camelot codebase follows modern C++ best practices while maintaining clear, organized, and maintainable code structure.