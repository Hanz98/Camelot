add_executable(Pendragon
    src/main.cpp
)

target_include_directories(Pendragon PUBLIC ${PROJECT_SOURCE_DIR})


target_link_libraries(Pendragon PRIVATE Camelot)
target_link_libraries(Pendragon PRIVATE Avalon)
#add_subdirectory("src")
