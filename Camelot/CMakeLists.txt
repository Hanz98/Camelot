add_subdirectory("API")
add_subdirectory("src")

set(Camelot_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Camelot INTERFACE)

target_include_directories(Camelot INTERFACE ${Camelot_include_dirs})

target_link_libraries(Camelot INTERFACE Avalon)
