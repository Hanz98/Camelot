add_library(Avalon_interface_window
        "GlfWrapper.cpp"
        "GlfWrapper.h"
        "IGlfWrapper.h"
)

target_include_directories(Avalon_interface_window PUBLIC ${glfw3_INCLUDE_DIRS})

target_link_libraries(Avalon_interface_window
    PUBLIC
        glfw
)

message ("Include dirs: " ${glfw3_INCLUDE_DIRS})
