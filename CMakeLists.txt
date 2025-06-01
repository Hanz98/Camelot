cmake_minimum_required(VERSION 3.15)

project(Camelot)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/linux-clang-toolchain.cmake")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS True )
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows-toolchain.cmake")
    message(STATUS "CMAKE_TOOLCHAIN_FILE set to: ${CMAKE_TOOLCHAIN_FILE}")
    message(STATUS "Windows toolchain file is being processed.")
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "" FORCE)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "" FORCE)
        endif()
    endif()
endif()


if(EXISTS "${CMAKE_BINARY_DIR}/generators/conan_paths.cmake")
  include("${CMAKE_BINARY_DIR}/generators/conan_paths.cmake")
endif()

#set(Camelot_external_dirs ${PROJECT_SOURCE_DIR}/external/)
set(Camelot_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})

include(CTest)

add_subdirectory(Avalon)
add_subdirectory(Camelot)
#add_subdirectory(Excalibur)
#add_subdirectory(Merlin )
#add_subdirectory(Nimue)
add_subdirectory(Pendragon)

#add_subdirectory(Tests)
