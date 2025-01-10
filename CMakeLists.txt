cmake_minimum_required(VERSION 3.15)

project(Camelot VERSION 1.0)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(CTest)

add_subdirectory(Avalon)
add_subdirectory(Camelot)
add_subdirectory(Excalibur)
add_subdirectory(Merlin )
add_subdirectory(Nimue)
add_subdirectory(Pendragon)

add_subdirectory(Tests)