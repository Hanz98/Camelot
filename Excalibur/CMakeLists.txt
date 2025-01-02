
add_library(Excalibur SHARED
"main/Excalibur.h"
"main/Excalibur.cpp"
)

target_include_directories(Excalibur PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(Excalibur PUBLIC Camelot_api_main)
