add_executable(Pendragon
    src/main.cpp
)

target_include_directories(Pendragon PUBLIC ${PROJECT_SOURCE_DIR})

# Add data subdirectory
add_subdirectory("src/data")

target_link_libraries(Pendragon PRIVATE Camelot)
target_link_libraries(Pendragon PRIVATE Avalon)
target_link_libraries(Pendragon PRIVATE Pendragon_data)
#add_subdirectory("src")
