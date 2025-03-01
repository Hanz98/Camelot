add_Library(Avalon_main
"Avalon.cpp"
"Avalon.h"
)

target_include_directories(Avalon_main PUBLIC ${Avalon_include_dirs})
target_link_libraries(Avalon_main Avalon
                                  Avalon_window
                                  Avalon_device
                                  spdlog::spdlog)
