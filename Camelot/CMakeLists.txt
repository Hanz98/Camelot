add_subdirectory("API")
add_subdirectory("src")

set(Camelot_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Camelot
    SHARED
        "main.cpp"
)

target_include_directories(Camelot PUBLIC ${Camelot_include_dirs})

target_link_libraries(Camelot
    PUBLIC
        Avalon
        Camelot_main
)
