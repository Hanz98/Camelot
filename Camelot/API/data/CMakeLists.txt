add_library(Camelot_data_api INTERFACE)

target_include_directories(Camelot_data_api INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(Camelot_data_api PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)