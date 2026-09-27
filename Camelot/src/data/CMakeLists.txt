# Data ingestion for MCAP replay: the protobuf wire reader, the Foxglove
# message decoders, the MCAP source, the transform tree, the playback clock
# and the JPEG decoder. Nothing here touches Vulkan.
#
# mcap is header-only; McapSource.cpp is the one translation unit that
# defines MCAP_IMPLEMENTATION, and it needs lz4 and zstd for the chunks.

find_package(glm REQUIRED)
find_package(libjpeg-turbo REQUIRED)
find_package(lz4 REQUIRED)
find_package(mcap REQUIRED)
find_package(zstd REQUIRED)

add_library(Camelot_data
    "FoxgloveDecoder.cpp"
    "FoxgloveDecoder.h"
    "FoxgloveMessages.cpp"
    "FoxgloveMessages.h"
    "JpegDecoder.cpp"
    "JpegDecoder.h"
    "McapSource.cpp"
    "McapSource.h"
    "Playback.cpp"
    "Playback.h"
    "ProtoReader.cpp"
    "ProtoReader.h"
    "TransformTree.cpp"
    "TransformTree.h"
)

target_include_directories(Camelot_data PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(Camelot_data
    PUBLIC
        glm::glm
    PRIVATE
        libjpeg-turbo::turbojpeg-static
        LZ4::lz4_static
        mcap::mcap
        zstd::libzstd_static
)
