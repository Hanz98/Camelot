add_subdirectory("device")
add_subdirectory("main")
add_subdirectory("utils")
add_subdirectory("validation")
add_subdirectory("window")

add_library(Avalon_src INTERFACE)

target_link_libraries(Avalon_src INTERFACE Avalon_window)
