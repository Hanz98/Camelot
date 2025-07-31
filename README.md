# Camelot
![Build and Test status](https://github.com/Hanz98/Camelot/actions/workflows/build.yaml/badge.svg)

## Setup project 

### Linux 

On Linux, run `setup.sh` file to install dependencies with Conan, and build the CMake project. 

### Windows 

On Windows, run `setup.ps1` file to generate a Visual Studio solution file. You should then be able to open the project folder in Visual Studio.

Also, make sure to install the CMake and Conan extensions to Visual Studio.

## Continuous Integration

The project uses GitHub Actions for continuous integration, building and testing on both Linux and Windows platforms:

### Linux CI (`ubuntu-latest`)
- Installs CMake, GCC, and Python dependencies
- Uses Conan for dependency management (gtest, glfw, spdlog, vk-bootstrap, volk)
- Installs Vulkan SDK via system packages (`libvulkan-dev`)
- Installs Vulkan Memory Allocator manually
- Builds with Unix Makefiles generator
- Runs all tests

### Windows CI (`windows-latest`)
- Automatically downloads and installs Vulkan SDK 1.3.280.0
- Uses Chocolatey for Python and Conan installation
- Installs dependencies via Conan with Windows-specific configuration
- Installs Vulkan Memory Allocator to Vulkan SDK directory
- Builds with Visual Studio 2022 generator (x64)
- Runs all tests

Both platforms use caching to optimize build times (Vulkan SDK, Conan dependencies).

