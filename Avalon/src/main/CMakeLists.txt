add_library(Avalon_main
"Avalon.cpp"
"Avalon.h"
)

target_include_directories(
    Avalon_main
        PUBLIC
             ${Avalon_include_dirs}
)

target_link_libraries(
    Avalon_main
        PUBLIC
            Avalon
            Avalon_window
            Avalon_device
            Avalon_allocator
            Avalon_presentation_image
            Avalon_presentation_swapchain
            Avalon_renderer
            spdlog::spdlog
)
