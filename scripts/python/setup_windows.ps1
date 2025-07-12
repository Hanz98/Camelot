# Setup script for Python development environment on Windows

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootDir = Split-Path -Parent (Split-Path -Parent $scriptDir)

Set-Location $rootDir

Write-Host "Setting up Python development environment for Camelot..." -ForegroundColor Green

# Check Python version
python --version

# Create virtual environment if it doesn't exist
if (!(Test-Path "venv")) {
    Write-Host "Creating virtual environment..." -ForegroundColor Yellow
    python -m venv venv
}

# Activate virtual environment
Write-Host "Activating virtual environment..." -ForegroundColor Yellow
& "venv\Scripts\Activate.ps1"

# Upgrade pip
Write-Host "Upgrading pip..." -ForegroundColor Yellow
python -m pip install --upgrade pip

# Install dependencies
Write-Host "Installing Python dependencies..." -ForegroundColor Yellow
pip install -r requirements.txt

Write-Host "Python development environment setup complete!" -ForegroundColor Green
Write-Host ""
Write-Host "To activate the virtual environment, run:" -ForegroundColor Cyan
Write-Host "  venv\Scripts\Activate.ps1" -ForegroundColor White
Write-Host ""
Write-Host "To run the Python app, run:" -ForegroundColor Cyan
Write-Host "  python python_app\main.py" -ForegroundColor White
Write-Host ""
Write-Host "To deactivate the virtual environment, run:" -ForegroundColor Cyan
Write-Host "  deactivate" -ForegroundColor White