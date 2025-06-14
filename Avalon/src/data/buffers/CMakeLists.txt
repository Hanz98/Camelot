add_library(Avalon_buffers
"Window.cpp"
"Window.h"
"Surface.cpp"
"Surface.h"
"SurfaceManager.cpp"
"SurfaceManager.h"
)

target_include_directories(Avalon_buffers PUBLIC ${Avalon_include_dirs})
target_include_directories(Avalon_buffers PUBLIC ${glfw3_INCLUDE_DIRS})

target_link_libraries(Avalon_buffers PUBLIC  Avalon
                                            Avalon_interface_window
                                            glfw)
