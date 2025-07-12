# Development utility script for Python app on Windows

param(
    [string]$Command = "run"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootDir = Split-Path -Parent (Split-Path -Parent $scriptDir)

Set-Location $rootDir

# Check if virtual environment exists
if (!(Test-Path "venv")) {
    Write-Host "Virtual environment not found. Please run setup_windows.ps1 first." -ForegroundColor Red
    exit 1
}

# Activate virtual environment
& "venv\Scripts\Activate.ps1"

switch ($Command.ToLower()) {
    "run" {
        Write-Host "Running Python application..." -ForegroundColor Green
        python python_app\main.py
    }
    "test" {
        Write-Host "Running tests..." -ForegroundColor Green
        if (Test-Path "tests") {
            pytest tests\
        } else {
            Write-Host "No tests directory found. Skipping tests." -ForegroundColor Yellow
        }
    }
    "lint" {
        Write-Host "Running code formatting and linting..." -ForegroundColor Green
        black python_app\
        flake8 python_app\
        mypy python_app\
    }
    "format" {
        Write-Host "Formatting code..." -ForegroundColor Green
        black python_app\
    }
    "build-desktop" {
        Write-Host "Building desktop executable..." -ForegroundColor Green
        pyinstaller --onefile --windowed python_app\main.py --name camelot
    }
    "build-android" {
        Write-Host "Building Android APK..." -ForegroundColor Green
        Write-Host "Android builds are not supported on Windows with buildozer." -ForegroundColor Yellow
        Write-Host "Please use WSL or a Linux environment for Android builds." -ForegroundColor Yellow
    }
    "clean" {
        Write-Host "Cleaning build artifacts..." -ForegroundColor Green
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build, dist, *.spec
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue .buildozer
        Get-ChildItem -Recurse -Name "*.pyc" | Remove-Item -Force
        Get-ChildItem -Recurse -Directory -Name "__pycache__" | Remove-Item -Recurse -Force
    }
    default {
        Write-Host "Usage: .\dev.ps1 [run|test|lint|format|build-desktop|build-android|clean]" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Commands:" -ForegroundColor Cyan
        Write-Host "  run           - Run the Python application (default)" -ForegroundColor White
        Write-Host "  test          - Run tests" -ForegroundColor White
        Write-Host "  lint          - Run code formatting and linting" -ForegroundColor White
        Write-Host "  format        - Format code with black" -ForegroundColor White
        Write-Host "  build-desktop - Build desktop executable with PyInstaller" -ForegroundColor White
        Write-Host "  build-android - Build Android APK (Linux only)" -ForegroundColor White
        Write-Host "  clean         - Clean build artifacts" -ForegroundColor White
        exit 1
    }
}