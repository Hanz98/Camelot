add_library(Avalon_utils INTERFACE
"Initializers.hpp"
)

target_include_directories(Avalon_utils INTERFACE ${Avalon_include_dirs})

target_link_libraries(Avalon_main Avalon)
