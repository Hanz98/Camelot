add_executable(Pendragon
    src/main.cpp
    src/mcap/McapReader.cpp
)

target_include_directories(Pendragon PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Pendragon PRIVATE src)

# Temporarily commenting out dependencies for MCAP testing
# target_link_libraries(Pendragon PRIVATE Camelot)
# Temporarily commenting out Avalon dependency for MCAP testing
# target_link_libraries(Pendragon PRIVATE Avalon)
#add_subdirectory("src")
