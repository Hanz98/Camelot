add_library(Avalon_interface_window
        "GlfWrapper.cpp"
        "GlfWrapper.h"
        "IGlfWrapper.h"
)

target_link_libraries(Avalon_interface_window
    PUBLIC
        glfw
)
