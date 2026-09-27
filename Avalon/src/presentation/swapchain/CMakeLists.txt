add_library(Avalon_presentation_swapchain
"SwapchainModel.cpp"
"SwapchainModel.h"
)

target_include_directories(Avalon_presentation_swapchain
    PUBLIC
        ${Avalon_include_dirs}
)

target_link_libraries(Avalon_presentation_swapchain
    PUBLIC
        Avalon
        Avalon_device
        Avalon_window
        Avalon_presentation_image
        Avalon_presentation_renderpass
        spdlog::spdlog
)
