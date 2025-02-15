add_library(Avalon_device
"Device.cpp"
"Device.h"
"Instance.cpp"
"Instance.h"
)


target_include_directories(Avalon_device PUBLIC ${Avalon_include_dirs})
target_include_directories(Avalon_device PUBLIC ${Camelot_include_dirs})
