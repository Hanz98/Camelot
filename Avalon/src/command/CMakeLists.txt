add_library(Avalon_command
"CommandPool.cpp"
"CommandPool.h"
)

target_include_directories(Avalon_command PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_command
    PUBLIC
        Avalon
        Avalon_device
        spdlog::spdlog
)
