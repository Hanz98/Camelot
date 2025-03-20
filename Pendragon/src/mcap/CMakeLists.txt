add_library(Pendragon_mcap_reader
    McapReader.cpp
    McapReader.h
)

find_package(mcap REQUIRED)
find_package(protobuf REQUIRED)
# ...
target_link_libraries(Pendragon_mcap_reader
    PUBLIC
        protobuf::protobuf
        mcap::mcap
        spdlog::spdlog
)
