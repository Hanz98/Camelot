add_subdirectory("main")
add_subdirectory("data")

add_library(Camelot_API INTERFACE)

target_link_libraries(Camelot_API INTERFACE Camelot_data_api)
