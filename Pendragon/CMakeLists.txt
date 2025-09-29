add_executable(Pendragon
    src/main.cpp
)

find_package(spdlog REQUIRED)

target_include_directories(Pendragon
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${spdlog_INCLUDE_DIRS}
)


target_link_libraries(Pendragon
    PRIVATE
        Camelot
        spdlog::spdlog
)


#add_subdirectory("src")
