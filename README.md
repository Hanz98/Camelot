# Camelot
![Build and Test status](https://github.com/Hanz98/Camelot/actions/workflows/build.yaml/badge.svg)

## Setup project 

### Linux 

On Linux, run `setup.sh` file to install dependencies with Conan, and build the CMake project. 

### Windows 

On Windows, run `setup.ps1` file to generate a Visual Studio solution file. You should then be able to open the project folder in Visual Studio.

Also, make sure to install the CMake and Conan extensions to Visual Studio.

## Code Quality and Pre-commit Hooks

This project uses pre-commit hooks to enforce code quality standards based on the C++ Core Guidelines.

### Prerequisites

1. Install pre-commit:
   ```bash
   pip install pre-commit
   ```

2. Install clang-tidy (required for C++ Core Guidelines checks):
   ```bash
   # Ubuntu/Debian
   sudo apt install clang-tidy
   
   # macOS
   brew install llvm
   
   # Windows
   # Install LLVM from https://llvm.org/builds/
   ```

### Setting up pre-commit hooks

1. Install the pre-commit hooks:
   ```bash
   pre-commit install
   ```

2. Run pre-commit on all files (optional):
   ```bash
   pre-commit run --all-files
   ```

### Code Quality Checks

The pre-commit configuration includes:

- **License header insertion**: Automatically adds license headers to C++ source files
- **C++ Core Guidelines validation**: Uses clang-tidy to check for violations of C++ Core Guidelines including:
  - `cppcoreguidelines-avoid-magic-numbers`: Avoid magic numbers in code
  - `cppcoreguidelines-init-variables`: Initialize variables properly
  - `cppcoreguidelines-prefer-member-initializer`: Prefer member initializers
  - `modernize-use-nullptr`: Use nullptr instead of NULL
  - `modernize-use-auto`: Use auto where appropriate
  - `modernize-use-override`: Use override keyword

The hooks will run automatically on `git commit` and will prevent commits if violations are found.

### Manual execution

To run the hooks manually on specific files:
```bash
# Run all hooks on staged files
pre-commit run

# Run specific hook on all files
pre-commit run clang-tidy --all-files

# Run on specific files
pre-commit run --files path/to/file.cpp
```

