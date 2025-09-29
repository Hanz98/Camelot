from conan import ConanFile
from conan.tools.cmake import cmake_layout

class CamelotConan(ConanFile):
    name = "Camelot"
    version = "0.1.0"

    settings = "os", "arch", "compiler", "build_type"

    requires = (
        "gtest/1.15.0",
        "glfw/3.4",
        "spdlog/1.15.0",
        "vk-bootstrap/0.7",
        "vulkan-headers/1.3.239.0",
        "vulkan-loader/1.3.239.0",
    )

    generators = "CMakeDeps", "CMakeToolchain"

    def layout(self):
        cmake_layout(self)
