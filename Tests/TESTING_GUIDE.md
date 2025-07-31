# How to Extend the Camelot Test Suite

This guide explains how to add new tests to the Camelot test suite and follow best practices for testing.

## Quick Start

### Adding a Simple Unit Test

1. **Create test file**: `Tests/unit/module/test_new_feature.cpp`
2. **Write test**: Follow the existing patterns
3. **Update CMake**: Add file to module's `CMakeLists.txt`
4. **Run tests**: `./scripts/run_tests.sh`

### Test File Template

```cpp
/*
 * Copyright 2024 Jan Filip
 * Licensed under the MIT License.
 */

#include <gtest/gtest.h>
#include "ModuleName/Feature.h"

class FeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test fixtures
    }
    
    void TearDown() override {
        // Clean up resources
    }
    
    // Test fixtures and helper methods
};

TEST_F(FeatureTest, BasicFunctionality) {
    // Arrange
    auto feature = createFeature();
    
    // Act
    auto result = feature.doSomething();
    
    // Assert
    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.getValue(), expectedValue);
}

TEST_F(FeatureTest, ErrorHandling) {
    // Test error conditions
    EXPECT_THROW(feature.invalidOperation(), std::runtime_error);
}
```

## Test Categories

### 1. Unit Tests (`Tests/unit/`)

**Purpose**: Test individual components in isolation
**Location**: `Tests/unit/module/`
**Naming**: `test_component_name.cpp`

#### Example Structure:
```
Tests/unit/
├── avalon/          # Graphics/Vulkan tests
│   ├── test_instance.cpp
│   ├── test_device.cpp
│   └── test_validation.cpp
├── camelot/         # Core framework tests
│   └── test_core.cpp
└── common/          # Utility tests
    └── test_utils.cpp
```

### 2. Mock Infrastructure (`Tests/mocks/`)

**Purpose**: Provide controlled test environments
**Key Files**:
- `vulkan_mock.h`: Vulkan API mocking
- `mock_icd.cpp`: Mock Vulkan driver

#### Using Vulkan Mocks:

```cpp
#include "vulkan_mock.h"

class VulkanComponentTest : public VulkanTestFixture {
    // Automatic mock setup
};

TEST_F(VulkanComponentTest, InstanceCreation) {
    // Configure mock behavior
    EXPECT_CALL(vulkanMock, CreateInstance(_, _, _))
        .WillOnce(Return(VK_SUCCESS));
    
    // Test your component
    auto instance = createVulkanInstance();
    EXPECT_TRUE(instance.isValid());
}
```

## Best Practices

### 1. Test Naming
- **Classes**: `ComponentNameTest`
- **Methods**: `MethodName_Condition_ExpectedBehavior`
- **Files**: `test_component_name.cpp`

```cpp
// Good examples:
TEST_F(InstanceTest, Init_WithValidParameters_CreatesInstance)
TEST_F(DeviceTest, SelectPhysicalDevice_NoDeviceAvailable_ThrowsException)
TEST_F(ValidationTest, DebugCallback_ErrorMessage_LogsAndContinues)
```

### 2. Test Structure (AAA Pattern)
```cpp
TEST_F(ComponentTest, TestName) {
    // Arrange - Set up test data and conditions
    auto component = createTestComponent();
    auto inputData = generateTestData();
    
    // Act - Execute the operation being tested
    auto result = component.processData(inputData);
    
    // Assert - Verify the expected outcome
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value, expectedValue);
}
```

### 3. Mock Usage Guidelines

#### When to Mock:
- External dependencies (Vulkan, file system, network)
- Slow operations (large file processing, complex calculations)
- Error injection scenarios
- Hardware-dependent functionality

#### Mock Configuration:
```cpp
class ComponentTest : public VulkanTestFixture {
protected:
    void SetUp() override {
        VulkanTestFixture::SetUp();
        
        // Configure default mock behavior
        ON_CALL(vulkanMock, CreateInstance(_, _, _))
            .WillByDefault(Return(VK_SUCCESS));
    }
};
```

### 4. Error Testing
Always test both success and failure paths:

```cpp
TEST_F(ComponentTest, Operation_Success) {
    // Test normal operation
}

TEST_F(ComponentTest, Operation_InvalidInput_ThrowsException) {
    // Test error handling
    EXPECT_THROW(component.operation(invalidInput), std::invalid_argument);
}

TEST_F(ComponentTest, Operation_SystemFailure_ReturnsError) {
    // Test system failure scenarios
    EXPECT_CALL(vulkanMock, CreateInstance(_, _, _))
        .WillOnce(Return(VK_ERROR_INITIALIZATION_FAILED));
        
    auto result = component.initializeVulkan();
    EXPECT_FALSE(result.success);
}
```

## Module-Specific Guidelines

### Avalon (Vulkan/Graphics) Tests

#### Key Test Areas:
- Instance lifecycle management
- Device selection and creation
- Resource allocation and cleanup
- Error handling and validation
- Debug callback functionality

#### Mock Setup:
```cpp
class AvalonTest : public VulkanTestFixture {
protected:
    void SetUp() override {
        VulkanTestFixture::SetUp();
        
        // Set up common Vulkan mock behavior
        setupMockPhysicalDevices();
        setupMockInstance();
    }
    
    void setupMockPhysicalDevices() {
        EXPECT_CALL(vulkanMock, EnumeratePhysicalDevices(_, _, _))
            .WillRepeatedly(DoAll(
                SetArgPointee<1>(1),  // Return 1 device
                Return(VK_SUCCESS)
            ));
    }
};
```

### Camelot (Core) Tests

#### Focus Areas:
- API interfaces
- State management
- Data structures
- Utility functions

#### Example:
```cpp
TEST_F(CamelotTest, API_InitializeFramework_SetsUpCorrectly) {
    auto result = Camelot::initialize();
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(Camelot::isInitialized());
}
```

## Advanced Testing Patterns

### 1. Parameterized Tests
For testing multiple similar scenarios:

```cpp
class ValidationTest : public ::testing::TestWithParam<VkResult> {
};

TEST_P(ValidationTest, VkCheckResult_HandlesAllErrorCodes) {
    VkResult errorCode = GetParam();
    
    if (errorCode == VK_SUCCESS) {
        EXPECT_NO_THROW(VK_CHECK_RESULT(mockFunction(errorCode)));
    } else {
        EXPECT_THROW(VK_CHECK_RESULT(mockFunction(errorCode)), std::runtime_error);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ErrorCodes,
    ValidationTest,
    ::testing::Values(VK_SUCCESS, VK_ERROR_INITIALIZATION_FAILED, VK_ERROR_OUT_OF_HOST_MEMORY)
);
```

### 2. Fixture Inheritance
For related test suites:

```cpp
class BaseVulkanTest : public VulkanTestFixture {
protected:
    void SetUp() override {
        VulkanTestFixture::SetUp();
        setupCommonMocks();
    }
    
    void setupCommonMocks() { /* common setup */ }
};

class InstanceTest : public BaseVulkanTest { /* specific tests */ };
class DeviceTest : public BaseVulkanTest { /* specific tests */ };
```

### 3. Custom Matchers
For complex assertions:

```cpp
MATCHER_P(HasErrorCode, expectedCode, "") {
    return arg.getErrorCode() == expectedCode;
}

TEST_F(ErrorTest, Function_InvalidInput_ReturnsSpecificError) {
    auto result = component.process(invalidInput);
    EXPECT_THAT(result, HasErrorCode(ERROR_INVALID_INPUT));
}
```

## Performance Considerations

### 1. Fast Tests
- Keep tests under 1ms when possible
- Use mocks instead of real I/O operations
- Avoid thread sleep or blocking operations

### 2. Test Isolation
- Each test should be independent
- Clean up resources in `TearDown()`
- Use fresh objects for each test

### 3. Parallel Execution
- Avoid global state modifications
- Use thread-safe test fixtures
- Consider test execution order independence

## Debugging Tests

### 1. Running Specific Tests
```bash
# Run single test
./scripts/run_tests.sh --filter="ComponentTest.SpecificTest"

# Run test suite
./scripts/run_tests.sh --filter="ComponentTest.*"

# Run with verbose output
./scripts/run_tests.sh --verbose
```

### 2. GDB Integration
```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Run test under GDB
gdb --args ./bin/unit_tests --gtest_filter="FailingTest"
```

### 3. Common Issues

#### Mock Not Working:
- Verify mock is properly linked
- Check that global mock pointer is set
- Ensure mock expectations are configured

#### Compilation Errors:
- Check include paths in CMakeLists.txt
- Verify header dependencies
- Ensure C++20 features are available

#### Runtime Failures:
- Add debug output to understand test flow
- Use ASSERT vs EXPECT appropriately
- Check resource cleanup in TearDown

## Integration with CI

Tests automatically run on every push/PR. To ensure CI success:

1. **All tests must pass locally**
2. **No external dependencies** (use mocks)
3. **Fast execution** (< 5 minutes total)
4. **Deterministic results** (no flaky tests)

### CI Configuration
Tests are configured in `.github/workflows/build.yaml`:
- Builds project with test support
- Runs all test suites
- Reports failures with detailed output
- Prevents merge if tests fail

## Future Enhancements

### Planned Improvements:
- Coverage reporting with gcov/lcov
- Memory leak detection with Valgrind
- Performance benchmarking
- Cross-platform testing (Windows/macOS)
- Integration test framework

### Contributing Guidelines:
- Write tests for all new features
- Maintain existing test coverage
- Update documentation for new patterns
- Follow naming conventions
- Ensure CI passes before requesting review