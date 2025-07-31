add_library(Pendragon_data
    McapReader.cpp
)

target_include_directories(Pendragon_data PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../API
)

find_package(mcap REQUIRED)
target_link_libraries(Pendragon_data PUBLIC mcap::mcap)

set_target_properties(Pendragon_data PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)