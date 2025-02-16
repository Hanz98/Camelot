add_library(Camelot_main
    "MainModel.h"
    "MainModel.cpp"
)

include_directories(${Camelot_include_dirs})

target_include_directories(Camelot_main PRIVATE ${Camelot_include_dirs} ${glfw3_INCLUDE_DIRS})
target_link_libraries(Camelot_main PRIVATE Avalon)
