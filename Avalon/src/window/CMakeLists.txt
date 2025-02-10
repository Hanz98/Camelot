add_library(Avalon_window
"Window.cpp"
"Window.h"
"WindowManager.cpp"
"WindowManager.h"
)

target_include_directories(Avalon_window PUBLIC ${Avalon_include_dirs})
target_include_directories(Avalon_window PUBLIC ${glfw3_INCLUDE_DIRS})
target_include_directories(Avalon_window PUBLIC ${glog_INCLUDE_DIRS})

target_link_libraries(Avalon_window PUBLIC glfw
                                           glog::glog)

message("Avalon_window added")
