add_library(Avalon_device
"Device.cpp"
"Device.h"
"Instance.cpp"
"Instance.h"
)

find_package(vk-bootstrap REQUIRED)
target_include_directories(Avalon_device
    PUBLIC
     ${Avalon_include_dirs}
     ${Camelot_include_dirs}
     ${vk-bootstrap_INCLUDE_DIRS}
)



target_link_libraries(Avalon_device PRIVATE Avalon
                                            glfw
                                            spdlog::spdlog
                                            vk-bootstrap::vk-bootstrap
                                            Vulkan::Vulkan
)
