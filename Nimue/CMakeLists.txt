add_library(Nimue INTERFACE
"API/Nimue.h"
"API/Nimue.cpp"
)

add_subdirectory("API")
add_subdirectory("src")

target_include_directories(Nimue INTERFACE
    ${PROJECT_SOURCE_DIR}
)
