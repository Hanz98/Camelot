add_library(Avalon_utils
"Initializers.hpp"
"ResourceDescriptor.h"
"ResourceDescriptor.cpp"
)

target_include_directories(Avalon_utils PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_utils PUBLIC Avalon)
