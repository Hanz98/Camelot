add_subdirectory("API")
add_subdirectory("src")

set(Camelot_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Camelot
    SHARED
        "main.cpp"
)

target_include_directories(Camelot PUBLIC ${Camelot_include_dirs})
# MSVC only emits an import library for a DLL that exports something; without it
# Pendragon fails to link (LNK1181: cannot open input file 'lib\Camelot.lib').
set_target_properties(Camelot PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_link_libraries(Camelot
    PUBLIC
        Avalon
        Camelot_main
)
