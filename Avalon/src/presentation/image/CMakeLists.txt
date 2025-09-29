add_library(Avalon_presentation_image
"Image.cpp"
"Image.h"
)

target_include_directories(Avalon_presentation_image
    PUBLIC
        ${Avalon_include_dirs}
)

target_link_libraries(Avalon_presentation_image
    PUBLIC
        Avalon
        spdlog::spdlog
)
