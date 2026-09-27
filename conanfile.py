from conan import ConanFile
from conan.tools.cmake import cmake_layout

class CamelotConan(ConanFile):
    name = "Camelot"
    version = "0.1.0"

    settings = "os", "arch", "compiler", "build_type"

    requires = (
        "gtest/1.18.0",
        "glfw/3.4",
        "spdlog/1.17.0",
        "vk-bootstrap/1.4.350",
        "vulkan-headers/1.4.350.0",
        "vulkan-loader/1.4.350.0",
        "vulkan-memory-allocator/3.3.0",
    )

    # Build-time tools. glslang provides glslangValidator, which
    # cmake/CamelotShaders.cmake uses to compile GLSL to SPIR-V; its bindir is
    # exported to CMake through CMAKE_PROGRAM_PATH by the toolchain.
    tool_requires = ("glslang/1.4.350.0",)

    generators = "CMakeDeps", "CMakeToolchain"

    def layout(self):
        cmake_layout(self)
