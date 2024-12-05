add_subdirectory("src")
add_subdirectory("API")

add_library(Camelot_Lib INTERFACE)


target_link_libraries(Camelot_Lib INTERFACE CamelotLib_src)