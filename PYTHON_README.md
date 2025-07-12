# Camelot Python Development Environment

This repository now includes a Python development environment for creating cross-platform desktop applications that work on Windows, Linux, and Android.

## Overview

The Python app is built using [Kivy](https://kivy.org/), a powerful cross-platform framework that allows you to develop applications that run on:
- **Windows** (desktop)
- **Linux** (desktop)  
- **Android** (mobile)

## Quick Start

### Prerequisites

- **Python 3.8+** installed on your system
- **Git** for version control

### Linux Setup

1. Clone the repository (if not already done)
2. Run the setup script:
   ```bash
   ./scripts/python/setup_linux.sh
   ```
3. Activate the virtual environment:
   ```bash
   source venv/bin/activate
   ```
4. Run the application:
   ```bash
   python python_app/main.py
   ```

### Windows Setup

1. Open PowerShell as Administrator
2. Run the setup script:
   ```powershell
   .\scripts\python\setup_windows.ps1
   ```
3. Activate the virtual environment:
   ```powershell
   venv\Scripts\Activate.ps1
   ```
4. Run the application:
   ```powershell
   python python_app\main.py
   ```

## Development Commands

### Linux

Use the development utility script:
```bash
# Run the application
./scripts/python/dev.sh run

# Run tests
./scripts/python/dev.sh test

# Format and lint code
./scripts/python/dev.sh lint

# Build desktop executable
./scripts/python/dev.sh build-desktop

# Build Android APK
./scripts/python/dev.sh build-android

# Clean build artifacts
./scripts/python/dev.sh clean
```

### Windows

Use the PowerShell development script:
```powershell
# Run the application
.\scripts\python\dev.ps1 run

# Run tests
.\scripts\python\dev.ps1 test

# Format and lint code
.\scripts\python\dev.ps1 lint

# Build desktop executable
.\scripts\python\dev.ps1 build-desktop

# Clean build artifacts
.\scripts\python\dev.ps1 clean
```

## Project Structure

```
python_app/                 # Main Python application
├── __init__.py            # Package initialization
└── main.py               # Main application entry point

scripts/python/            # Development scripts
├── setup_linux.sh        # Linux environment setup
├── setup_windows.ps1     # Windows environment setup
├── dev.sh                # Linux development utilities
└── dev.ps1               # Windows development utilities

tests/                     # Test suite
├── __init__.py
└── test_main.py          # Main application tests

requirements.txt           # Python dependencies
pyproject.toml            # Project configuration
buildozer.spec            # Android build configuration
```

## Building for Different Platforms

### Desktop Applications

For creating standalone desktop executables:

**Linux/Windows:**
```bash
# Install PyInstaller (included in requirements.txt)
pip install pyinstaller

# Build executable
pyinstaller --onefile --windowed python_app/main.py --name camelot
```

The executable will be created in the `dist/` directory.

### Android Applications

For building Android APKs:

**Prerequisites (Linux only):**
- Java Development Kit (JDK)
- Android SDK
- Android NDK
- Buildozer

**Build process:**
```bash
# Install buildozer (included in requirements.txt)
pip install buildozer

# Initialize buildozer (first time only)
buildozer init

# Build debug APK
buildozer android debug

# Build release APK (requires signing)
buildozer android release
```

**Note:** Android builds are only supported on Linux. Windows users should use WSL (Windows Subsystem for Linux) or a Linux virtual machine.

## Development Workflow

1. **Setup:** Run the appropriate setup script for your platform
2. **Develop:** Edit files in `python_app/`
3. **Test:** Run `./scripts/python/dev.sh test` (Linux) or `.\scripts\python\dev.ps1 test` (Windows)
4. **Format:** Run the lint command to ensure code quality
5. **Build:** Create executables for your target platforms

## Configuration

### Python Dependencies

Core dependencies are listed in `requirements.txt`:
- **Kivy**: Cross-platform GUI framework
- **PyInstaller**: For creating desktop executables
- **Buildozer**: For creating Android APKs
- **Development tools**: pytest, black, flake8, mypy

### Android Configuration

The `buildozer.spec` file contains Android-specific configuration:
- App name and package information
- Required permissions
- Target Android API levels
- Supported architectures

## Troubleshooting

### Common Issues

1. **Virtual environment not activating:**
   - Ensure you ran the setup script first
   - Check Python installation and PATH

2. **Kivy installation fails:**
   - Install system dependencies (Linux): `sudo apt-get install python3-dev`
   - Update pip: `pip install --upgrade pip`

3. **Android build fails:**
   - Ensure you're on Linux (not Windows)
   - Install Android SDK and NDK
   - Check buildozer.spec configuration

### Getting Help

- Check the [Kivy documentation](https://kivy.org/doc/stable/)
- Review [Buildozer documentation](https://buildozer.readthedocs.io/) for Android builds
- Check existing issues in this repository

## Contributing

1. Follow the existing code style (enforced by black and flake8)
2. Add tests for new features
3. Update documentation as needed
4. Run the full test suite before submitting changes

## License

This project is licensed under the MIT License - see the LICENSES directory for details.