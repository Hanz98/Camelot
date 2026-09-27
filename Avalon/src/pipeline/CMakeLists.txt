add_library(Avalon_pipeline
  "GraphicsPipeline.cpp"
  "GraphicsPipeline.h"
  "PipelineLayout.cpp"
  "PipelineLayout.h"
  "PipelineManager.cpp"
  "PipelineManager.h"
)

target_include_directories(Avalon_pipeline PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_pipeline
    PUBLIC
        Avalon
        Avalon_device
        Avalon_shader
        spdlog::spdlog
)
