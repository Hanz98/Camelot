# linux-clang-toolchain.cmake
message(STATUS "Using linux-clang-toolchain.cmake")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)


# Specify the C and C++ compilers
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# Optionally, set the linker
# set(CMAKE_LINKER ld.lld) # If you want to use lld linker

# Optionally, set the sysroot if you are cross-compiling
# set(CMAKE_SYSROOT /path/to/sysroot)

# General compiler flags (apply to all build types)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -DLINUX")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++20 -DLINUX")

# Optionally, set linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld") # If you want to use lld linker

# Set optimization levels for Release and Debug builds
# CMAKE_BUILD_TYPE is not used directly in this file, but it is used
# indirectly through the following variables. Do not remove CMAKE_BUILD_TYPE
# even though CMake complains about it not being used!

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
