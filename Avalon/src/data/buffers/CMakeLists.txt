add_library(Avalon_buffers
"Buffer.cpp"
"Buffer.h"
)

target_include_directories(Avalon_buffers PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_buffers
    PUBLIC
        Avalon
        Avalon_allocator
        spdlog::spdlog
)
