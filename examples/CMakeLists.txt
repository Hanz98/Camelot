add_executable(mcap_reader_example
    mcap_reader_example.cpp
)

add_executable(mcap_summary
    mcap_summary.cpp
)

target_link_libraries(mcap_reader_example PRIVATE Pendragon_data)
target_link_libraries(mcap_summary PRIVATE Pendragon_data)

set_target_properties(mcap_reader_example PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(mcap_summary PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)