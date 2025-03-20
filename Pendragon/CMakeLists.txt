add_executable(Pendragon
    src/main.cpp
)

find_package(spdlog REQUIRED)

target_include_directories(Pendragon PUBLIC ${PROJECT_SOURCE_DIR})


target_link_libraries(Pendragon
    PRIVATE
        Camelot
        Pendragon_mcap_reader
        Avalon
)

add_subdirectory("src/mcap")
