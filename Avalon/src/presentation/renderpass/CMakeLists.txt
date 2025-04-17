add_library(Avalon_presentation_renderpass
"RenderPass.cpp"
"RenderPass.h"
)

target_include_directories(Avalon_presentation_renderpass
    PUBLIC
        ${Avalon_include_dirs}
)

target_link_libraries(Avalon_presentation_renderpass
    PUBLIC
        Avalon
        spdlog::spdlog
)
