add_library(Avalon_presentation_swapchain
"SwapChainModel.cpp"
"SwapChainModel.h"
)

target_include_directories(Avalon_presentation_swapchain
    PUBLIC
        ${Avalon_include_dirs}
)

target_link_libraries(Avalon_presentation_swapchain
    PUBLIC
        Avalon
        spdlog::spdlog
)
