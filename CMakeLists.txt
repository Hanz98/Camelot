cmake_minimum_required(VERSION 3.15)

project(Camelot VERSION 1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#if (MSVC)
#  add_compile_options(/experimental:module)
#elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#  add_compile_options(
#    -fmodules-ts
#    -fcxx-modules
#    -fimplicit-modules
#    -fimplicit-module-maps
#  )
#elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#  add_compile_options(
#    -fmodules-ts
#    -fcxx-modules
#  )
#endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

#set(Camelot_external_dirs ${PROJECT_SOURCE_DIR}/external/)
set(Camelot_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})

include(CTest)

add_subdirectory(Avalon)
add_subdirectory(Camelot)
#add_subdirectory(Excalibur)
#add_subdirectory(Merlin )
#add_subdirectory(Nimue)
add_subdirectory(Pendragon)

add_subdirectory(Tests)
