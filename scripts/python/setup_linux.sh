#!/bin/bash
# Setup script for Python development environment on Linux

set -e

scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rootDir="$(dirname "$(dirname "$scriptDir")")"

cd "$rootDir"

echo "Setting up Python development environment for Camelot..."

# Check Python version
python3 --version

# Create virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

# Activate virtual environment
echo "Activating virtual environment..."
source venv/bin/activate

# Upgrade pip
echo "Upgrading pip..."
pip install --upgrade pip

# Install dependencies
echo "Installing Python dependencies..."
pip install -r requirements.txt

echo "Python development environment setup complete!"
echo ""
echo "To activate the virtual environment, run:"
echo "  source venv/bin/activate"
echo ""
echo "To run the Python app, run:"
echo "  python python_app/main.py"
echo ""
echo "To deactivate the virtual environment, run:"
echo "  deactivate"