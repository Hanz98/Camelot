# Camelot
![Build and Test status](https://github.com/Hanz98/Camelot/actions/workflows/build.yaml/badge.svg)

Camelot is a cross-platform development environment that supports both C++ and Python application development.

## C++ Development

### Setup project 

#### Linux 

On Linux, run `setup.sh` file to install dependencies with Conan, and build the CMake project. 

#### Windows 

On Windows, run `setup.ps1` file to generate a Visual Studio solution file. You should then be able to open the project folder in Visual Studio.

Also, make sure to install the CMake and Conan extensions to Visual Studio.

## Python Development

### Cross-Platform Desktop Application

Camelot now includes a Python development environment for creating cross-platform desktop applications using Kivy framework. The Python app supports:

- **Windows** (desktop)
- **Linux** (desktop)
- **Android** (mobile)

### Quick Start

See [PYTHON_README.md](PYTHON_README.md) for detailed Python development instructions.

#### Linux Setup
```bash
./scripts/python/setup_linux.sh
./scripts/python/dev.sh run
```

#### Windows Setup
```powershell
.\scripts\python\setup_windows.ps1
.\scripts\python\dev.ps1 run
```

### Python Project Structure

```
python_app/                 # Main Python application
scripts/python/            # Python development scripts
tests/                     # Python test suite
requirements.txt           # Python dependencies
pyproject.toml            # Python project configuration
buildozer.spec            # Android build configuration
```

