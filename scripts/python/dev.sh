#!/bin/bash
# Development utility script for Python app

set -e

scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rootDir="$(dirname "$(dirname "$scriptDir")")"

cd "$rootDir"

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "Virtual environment not found. Please run setup_linux.sh first."
    exit 1
fi

# Activate virtual environment
source venv/bin/activate

case "${1:-run}" in
    "run")
        echo "Running Python application..."
        python python_app/main.py
        ;;
    "test")
        echo "Running tests..."
        if [ -d "tests" ]; then
            pytest tests/
        else
            echo "No tests directory found. Skipping tests."
        fi
        ;;
    "lint")
        echo "Running code formatting and linting..."
        black python_app/
        flake8 python_app/
        mypy python_app/
        ;;
    "format")
        echo "Formatting code..."
        black python_app/
        ;;
    "build-desktop")
        echo "Building desktop executable..."
        pyinstaller --onefile --windowed python_app/main.py --name camelot
        ;;
    "build-android")
        echo "Building Android APK..."
        if [ ! -f "buildozer.spec" ]; then
            echo "Initializing buildozer configuration..."
            buildozer init
            echo "Please edit buildozer.spec and run this command again."
        else
            buildozer android debug
        fi
        ;;
    "clean")
        echo "Cleaning build artifacts..."
        rm -rf build/ dist/ *.spec
        rm -rf .buildozer/
        find . -name "*.pyc" -delete
        find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
        ;;
    *)
        echo "Usage: $0 [run|test|lint|format|build-desktop|build-android|clean]"
        echo ""
        echo "Commands:"
        echo "  run           - Run the Python application (default)"
        echo "  test          - Run tests"
        echo "  lint          - Run code formatting and linting"
        echo "  format        - Format code with black"
        echo "  build-desktop - Build desktop executable with PyInstaller"
        echo "  build-android - Build Android APK with Buildozer"
        echo "  clean         - Clean build artifacts"
        exit 1
        ;;
esac