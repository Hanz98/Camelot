add_library(Avalon_renderer
  "IDrawable.h"
  "IPrePass.h"
  "MeshDrawable.cpp"
  "MeshDrawable.h"
  "PointCloudDrawable.cpp"
  "PointCloudDrawable.h"
  "Renderer.cpp"
  "Renderer.h"
)

target_include_directories(Avalon_renderer PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_renderer
    PUBLIC
        Avalon
        Avalon_buffers
        Avalon_camera
        Avalon_command
        Avalon_descriptor
        Avalon_geometry
        Avalon_device
        Avalon_pipeline
        Avalon_shader
        Avalon_presentation_renderpass
        Avalon_presentation_swapchain
        Avalon_sync
        Avalon_window
        spdlog::spdlog
)
