add_subdirectory("main")
add_subdirectory("data")

add_library(Camelot_src INTERFACE)

target_link_libraries(Camelot_src INTERFACE Camelot_main Camelot_data)
