add_library(Avalon_camera
  "Camera.cpp"
  "Camera.h"
  "CameraController.cpp"
  "CameraController.h"
)

target_include_directories(Avalon_camera PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_camera
    PUBLIC
        Avalon
        Avalon_window
        glm::glm
        spdlog::spdlog
)
