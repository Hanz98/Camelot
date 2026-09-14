add_library(Avalon_shader
"ShaderModule.cpp"
"ShaderModule.h"
)

target_include_directories(Avalon_shader PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_shader
    PUBLIC
        Avalon
        Avalon_device
        Avalon_shaders
        spdlog::spdlog
)
