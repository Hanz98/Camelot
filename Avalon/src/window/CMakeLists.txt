add_library(Avalon_window
"Window.cpp"
"Window.h"
"WindowManager.cpp"
"WindowManager.h"
)

target_include_directories(Avalon_window PUBLIC ${Avalon_include_dirs})

message("Avalon_window added")