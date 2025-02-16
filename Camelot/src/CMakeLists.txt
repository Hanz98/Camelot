add_subdirectory("main")

add_library(Camelot_src INTERFACE)

target_link_libraries(Camelot_src INTERFACE Camelot_main)
