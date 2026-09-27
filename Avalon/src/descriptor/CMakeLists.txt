add_library(Avalon_descriptor
  "DescriptorPool.cpp"
  "DescriptorPool.h"
  "DescriptorSetLayout.cpp"
  "DescriptorSetLayout.h"
)

target_include_directories(Avalon_descriptor PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_descriptor
    PUBLIC
        Avalon
        Avalon_device
        spdlog::spdlog
)
