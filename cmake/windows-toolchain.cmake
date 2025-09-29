# Windows toolchain file
cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_SYSTEM_PROCESSOR x64)

# Specify the compiler
set(CMAKE_C_COMPILER "cl.exe")
set(CMAKE_CXX_COMPILER "cl.exe")

# Set the Windows SDK version
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.22621.0")

# Set the target architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    #set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
    #set(CMAKE_VS_PLATFORM_TOOLSET "v143")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    #set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
    #set(CMAKE_VS_PLATFORM_TOOLSET "v143")
endif()

# Set the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the output directories
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add compiler flags
if(MSVC)
    add_compile_options(/W4 /WX /permissive- /Zc:__cplusplus /Zc:inline /Zc:preprocessor /Zc:externConstexpr /Zc:throwingNew /Zc:referenceBinding /Zc:strictStrings /Zc:sizedDealloc- /Zc:auto /Zc:lambda /Zc:threadSafeInit- /Zc:rvalueCast /Zc:noexceptTypes /Zc:char8_t /Zc:checkGwOdr /Zc:alignedNew /Zc:externC /Zc:templateScope /Zc:enumTypes /Zc:implicitNoexcept /Zc:static_assert /utf-8 /std:c++20 /EHsc)
    add_compile_options(/bigobj)
    add_compile_options(/MP)
    add_compile_options(/D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/D_SCL_SECURE_NO_WARNINGS)
    add_compile_options(/D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
    add_compile_options(/D_SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING)
    add_compile_options(/D_SILENCE_CXX20_CISO646_REMOVED_WARNING)
    add_compile_options(/D_SILENCE_CXX20_THREE_WAY_COMPARISON_DEPRECATION_WARNING)
    add_compile_options(/D_SILENCE_CXX20_CONSTEXPR_UNION_DEPRECATION_WARNING)
    add_compile_options(/D_SILENCE_CXX20_AGGREGATE_DEFAULT_INITIALIZER_DEPRECATION_WARNING)
    add_compile_options(/D_SILENCE_CXX20_IMPLICIT_MOVE_DEPRECATION_WARNING)
endif()
