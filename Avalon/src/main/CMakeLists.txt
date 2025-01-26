add_Library(Avalon_main
"Avalon.cpp"
"Avalon.h"
)

target_include_directories(Avalon_main PUBLIC ${Avalon_include_dirs})

message("Avalon_include_dirs: ${Avalon_include_dirs}")