add_library(Avalon_window
"Window.cpp"
"Window.h"
"Surface.cpp"
"Surface.h"
"SurfaceManager.cpp"
"SurfaceManager.h"
)

target_include_directories(Avalon_window PUBLIC ${Avalon_include_dirs})
target_include_directories(Avalon_window PUBLIC ${glfw3_INCLUDE_DIRS})

target_link_libraries(Avalon_window PUBLIC  Avalon
                                            Avalon_interface_window
                                            glfw)

message("Avalon_window added")
