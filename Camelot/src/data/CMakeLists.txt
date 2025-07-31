add_library(Camelot_data
    McapReader.cpp
)

target_include_directories(Camelot_data PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Camelot_include_dirs}
)

find_package(mcap REQUIRED)
target_link_libraries(Camelot_data PUBLIC mcap::mcap)

set_target_properties(Camelot_data PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)