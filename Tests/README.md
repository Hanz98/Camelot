# Camelot Unit Test Suite

This directory contains the comprehensive unit test suite for the Camelot project.

## Structure

```
Tests/
├── unit/                   # Unit tests organized by module
│   ├── avalon/            # Vulkan-related graphics tests
│   ├── camelot/           # Core framework tests
│   ├── common/            # Common utilities tests
│   └── test_runner.cpp    # Main test runner
├── mocks/                 # Mock infrastructure
│   ├── vulkan_mock.h      # Vulkan API mocking interfaces
│   └── mock_icd.cpp       # Mock Vulkan driver implementation
├── test_main.cpp          # Basic integration test
└── CMakeLists.txt         # Test build configuration
```

## Running Tests

### Prerequisites

1. **Build Dependencies**: CMake 3.15+, C++20 compiler
2. **Testing Framework**: Google Test (automatically handled by Conan)
3. **Mock Environment**: For Vulkan tests, mock ICD is used

### Building and Running Tests

#### Quick Test Run
```bash
# From project root
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
ctest --output-on-failure
```

#### Individual Test Suites
```bash
# Run basic integration test
./bin/test_main

# Run comprehensive unit tests
./bin/unit_tests

# Run with verbose output
./bin/unit_tests --gtest_filter="*" --gtest_verbose
```

#### Test Categories
```bash
# Run only Avalon (Vulkan) tests
./bin/unit_tests --gtest_filter="*Avalon*"

# Run only validation tests
./bin/unit_tests --gtest_filter="*CheckResult*:*ValidationLayers*"

# Run specific test case
./bin/unit_tests --gtest_filter="CheckResultTest.SuccessfulVulkanCall"
```

## Test Coverage

### Avalon Module (Graphics/Vulkan)
- ✅ **Error Handling**: VK_CHECK_RESULT macro testing
- ✅ **Validation**: Debug callback functionality
- 🚧 **Instance Management**: Vulkan instance lifecycle
- 🚧 **Device Management**: Physical device selection and logical device creation
- 🚧 **Resource Management**: Memory allocation and cleanup

### Camelot Module (Core)
- 🚧 **Core Framework**: Basic functionality tests
- 📋 **API Layer**: Public interface testing
- 📋 **State Management**: Application state handling

### Common Utilities
- ✅ **Basic Infrastructure**: Test framework validation
- 📋 **Helper Functions**: Utility function testing
- 📋 **Data Structures**: Core data type testing

**Legend**: ✅ Implemented, 🚧 In Progress, 📋 Planned

## Mock Environment

### Vulkan Mock ICD

The test suite includes a mock Vulkan driver implementation that allows testing Vulkan-dependent code without requiring actual GPU hardware or drivers.

#### Features
- **Instance Creation**: Mock Vulkan instance management
- **Device Enumeration**: Simulated physical device discovery
- **Debug Callbacks**: Validation layer message handling
- **Error Injection**: Controllable failure scenarios

#### Usage in Tests
```cpp
#include "vulkan_mock.h"

class MyVulkanTest : public VulkanTestFixture {
    // Automatic mock setup/teardown
};

TEST_F(MyVulkanTest, TestInstanceCreation) {
    EXPECT_CALL(vulkanMock, CreateInstance(_, _, _))
        .WillOnce(Return(VK_SUCCESS));
    
    // Test your Vulkan code here
}
```

## Adding New Tests

### 1. Create Test File
```cpp
// Tests/unit/module/test_new_feature.cpp
#include <gtest/gtest.h>
#include "module/NewFeature.h"

class NewFeatureTest : public ::testing::Test {
protected:
    void SetUp() override { /* setup */ }
    void TearDown() override { /* cleanup */ }
};

TEST_F(NewFeatureTest, TestBasicFunctionality) {
    // Your test implementation
    EXPECT_TRUE(true);
}
```

### 2. Update CMakeLists.txt
Add your test file to the appropriate module's `CMakeLists.txt`:
```cmake
set(MODULE_TEST_SOURCES
    existing_test.cpp
    test_new_feature.cpp  # Add this line
)
```

### 3. Test Best Practices
- **Focus on Unit Logic**: Test individual functions/methods in isolation
- **Mock External Dependencies**: Use mocks for Vulkan, file I/O, etc.
- **Test Error Paths**: Verify proper error handling
- **Keep Tests Fast**: Avoid slow operations, prefer mocks
- **Descriptive Names**: Use clear, specific test names

## Continuous Integration

Tests are automatically run on every push and pull request via GitHub Actions. The CI environment:

1. **Installs Dependencies**: CMake, GCC, Vulkan headers
2. **Sets Up Mock Environment**: Configures mock ICD
3. **Builds Project**: Compiles with test flags
4. **Runs Test Suite**: Executes all tests with coverage reporting
5. **Reports Results**: Provides detailed test output and coverage metrics

## Troubleshooting

### Common Issues

#### Build Failures
```bash
# Missing dependencies
sudo apt-get install cmake g++ build-essential libvulkan-dev

# Clean build
rm -rf build && mkdir build && cd build
```

#### Test Failures
```bash
# Run with debug output
./bin/unit_tests --gtest_verbose

# Run specific failing test
./bin/unit_tests --gtest_filter="FailingTest*"
```

#### Vulkan Mock Issues
```bash
# Verify mock ICD is properly loaded
export VK_ICD_FILENAMES=/path/to/mock_icd.json
export VK_LAYER_PATH=/path/to/mock/layers
```

## Performance

Tests are designed to be fast and deterministic:
- **Target Runtime**: < 5 seconds for full suite
- **No External Dependencies**: Tests run offline
- **Parallel Execution**: Tests can run concurrently
- **Minimal Setup**: Quick test environment initialization

## Future Enhancements

- **Coverage Reporting**: Integrate gcov/lcov for coverage metrics
- **Performance Benchmarks**: Add micro-benchmarks for critical paths
- **Integration Tests**: Full application testing with mock environments
- **Memory Leak Detection**: Valgrind integration for memory safety
- **Cross-Platform Testing**: Windows and macOS CI runners