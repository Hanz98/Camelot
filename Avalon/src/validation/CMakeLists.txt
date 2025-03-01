add_library(Avalon_validation
"CheckResult.h"
"ValidationLayers.cpp"
"ValidationLayers.h")

target_include_directories(Avalon_validation PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_validation Avalon
                                        spdlog::spdlog)
