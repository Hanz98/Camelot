add_library(Avalon_sync
"FrameSync.cpp"
"FrameSync.h"
)

target_include_directories(Avalon_sync PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_sync
    PUBLIC
        Avalon
        Avalon_device
        spdlog::spdlog
)
