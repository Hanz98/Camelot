import os

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout

class CamelotConan(ConanFile):
    name = "Camelot"
    version = "0.1.0"

    settings = "os", "arch", "compiler", "build_type"

    requires = (
        "gtest/1.18.0",
        "glfw/3.4",
        "glm/1.0.1",
        "libjpeg-turbo/3.2.0",
        "lz4/1.10.0",
        "mcap/2.1.3",
        "spdlog/1.17.0",
        "vk-bootstrap/1.4.350",
        "vulkan-headers/1.4.350.0",
        "vulkan-loader/1.4.350.0",
        "vulkan-memory-allocator/3.3.0",
        "zstd/1.5.7",
    )

    def requirements(self):
        # Dear ImGui with the docking branch; implot pins the matching
        # non-docking version, so force ours (same API).
        self.requires("imgui/1.92.5-docking", force=True)
        self.requires("implot/0.17")

    # Build-time tools. glslang provides glslangValidator, which
    # cmake/CamelotShaders.cmake uses to compile GLSL to SPIR-V; its bindir is
    # exported to CMake through CMAKE_PROGRAM_PATH by the toolchain.
    tool_requires = ("glslang/1.4.350.0",)

    def layout(self):
        cmake_layout(self)

    # The equivalent of generators = "CMakeDeps", "CMakeToolchain", with one
    # correction to the libjpeg-turbo package (see below).
    def generate(self):
        CMakeToolchain(self).generate()
        deps = CMakeDeps(self)
        self._use_lib64_where_the_libraries_are(
            self.dependencies["libjpeg-turbo"])
        deps.generate()

    @staticmethod
    def _use_lib64_where_the_libraries_are(dep):
        # conan-center's libjpeg-turbo recipe drops the toolchain block that
        # pins CMAKE_INSTALL_LIBDIR, so on hosts where CMake's GNUInstallDirs
        # picks lib64 (Arch, Fedora) the package ships lib64/ while its
        # cpp_info still says lib/ and find_package(libjpeg-turbo) fails.
        # Point the generated targets at the directory that exists.
        root = dep.package_folder
        if not root or os.path.isdir(os.path.join(root, "lib")):
            return
        lib64 = os.path.join(root, "lib64")
        if not os.path.isdir(lib64):
            return
        for info in [dep.cpp_info] + list(dep.cpp_info.components.values()):
            info.libdirs = [lib64]
