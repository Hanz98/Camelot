#!/bin/bash

# Setup script for Camelot test environment with mock ICD support
# This script configures the environment for running unit tests with Vulkan mocking

set -e

SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== Camelot Test Environment Setup ==="

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install basic dependencies
echo "📦 Installing dependencies..."
if command_exists apt-get; then
    sudo apt-get update
    sudo apt-get install -y cmake g++ build-essential libgtest-dev libvulkan-dev
elif command_exists brew; then
    brew install cmake googletest vulkan-headers
else
    echo "❌ Unsupported package manager. Please install dependencies manually."
    exit 1
fi

# Set up Vulkan SDK path
if [ -f /usr/include/vulkan/vulkan.h ]; then
    export VULKAN_SDK=/usr
    echo "✅ Vulkan SDK found at /usr"
elif [ -n "$VULKAN_SDK" ]; then
    echo "✅ Using existing VULKAN_SDK: $VULKAN_SDK"
else
    echo "⚠️  Vulkan SDK not found. Mock ICD will be used for testing."
fi

# Create mock ICD configuration
echo "🔧 Setting up mock ICD..."
MOCK_DIR="$PROJECT_ROOT/Tests/mock_icd"
mkdir -p "$MOCK_DIR"

# Create mock ICD manifest
cat > "$MOCK_DIR/mock_icd.json" << EOF
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "./libMockICD.so",
        "api_version": "1.3.0"
    }
}
EOF

# Set environment variables for mock ICD
export VK_ICD_FILENAMES="$MOCK_DIR/mock_icd.json"
export VK_DRIVER_FILES="$MOCK_DIR/mock_icd.json"

echo "🏗️  Building test suite..."
cd "$PROJECT_ROOT"

# Create build directory
BUILD_DIR="$PROJECT_ROOT/build_tests"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake for tests
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBUILD_TESTING=ON \
    -DVULKAN_SDK="${VULKAN_SDK:-/usr}"

# Build the project
cmake --build . --parallel $(nproc)

echo ""
echo "✅ Test environment setup complete!"
echo ""
echo "📋 Available test commands:"
echo "   ctest --output-on-failure              # Run all tests"
echo "   ./bin/test_main                        # Run basic tests"
echo "   ./bin/unit_tests                       # Run unit tests (if available)"
echo ""
echo "🔧 Environment variables set:"
echo "   VULKAN_SDK=$VULKAN_SDK"
echo "   VK_ICD_FILENAMES=$VK_ICD_FILENAMES"
echo ""
echo "📚 For more information, see Tests/README.md"