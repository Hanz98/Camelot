add_library(Avalon_validation
"CheckResult.h"
"ValidationLayers.cpp"
"ValidationLayers.h")

target_include_directories(Avalon_validation PUBLIC ${Avalon_include_dirs})
