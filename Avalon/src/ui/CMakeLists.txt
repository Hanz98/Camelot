# Dear ImGui + ImPlot with the GLFW and Vulkan backends, plus the widgets built
# on them. The backend sources come from the imgui Conan package
# (IMGUI_BINDINGS_DIR, found in Avalon/CMakeLists.txt).
add_library(Avalon_ui
  "GraphWidget.cpp"
  "GraphWidget.h"
  "RenderTarget.cpp"
  "RenderTarget.h"
  "SceneViewWidget.cpp"
  "SceneViewWidget.h"
  "TestPatternSource.cpp"
  "TestPatternSource.h"
  "TreeWidget.cpp"
  "TreeWidget.h"
  "UiContext.cpp"
  "UiContext.h"
  "VideoSource.h"
  "VideoTexture.cpp"
  "VideoTexture.h"
  "VideoWidget.cpp"
  "VideoWidget.h"
  "${IMGUI_BINDINGS_DIR}/imgui_impl_glfw.cpp"
  "${IMGUI_BINDINGS_DIR}/imgui_impl_vulkan.cpp"
)

target_include_directories(Avalon_ui PUBLIC ${Avalon_include_dirs} ${IMGUI_BINDINGS_DIR})

target_link_libraries(Avalon_ui
    PUBLIC
        Avalon
        Avalon_buffers
        Avalon_camera
        Avalon_descriptor
        Avalon_device
        Avalon_pipeline
        Avalon_presentation_image
        Avalon_renderer
        Avalon_window
        glfw
        glm::glm
        imgui::imgui
        implot::implot
        spdlog::spdlog
)
