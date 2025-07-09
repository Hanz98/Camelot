add_library(Avalon_device
"Device.cpp"
"Device.h"
"IModel.h"
"Instance.cpp"
"Instance.h"
"InstanceIModel.h"
)


target_include_directories(Avalon_device PUBLIC ${Avalon_include_dirs})
target_include_directories(Avalon_device PUBLIC ${Camelot_include_dirs})
include_directories(${spdlog_INCLUDE_DIRS})


target_link_libraries(Avalon_device PRIVATE glfw
                                            spdlog::spdlog
                                            vk-bootstrap::vk-bootstrap)
