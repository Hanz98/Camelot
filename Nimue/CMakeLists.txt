add_library(Nimue SHARED
"API/Nimue.h"
"API/Nimue.cpp"
)

add_subdirectory("API")
add_subdirectory("src")

target_include_directories(Nimue PUBLIC
    ${PROJECT_SOURCE_DIR}
)
