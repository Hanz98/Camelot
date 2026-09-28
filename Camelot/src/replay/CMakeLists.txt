# Replay of an MCAP recording: Recording (file, transforms, playback clock,
# message decoding into a sink) and SceneUpdater (the sink that keeps the
# Avalon renderer in sync). Recording has no Vulkan dependency; SceneUpdater
# links the renderer and UI layers for its drawables and video textures.

find_package(glm REQUIRED)
find_package(spdlog REQUIRED)

add_library(Camelot_replay
    "IRecordingSink.h"
    "Recording.cpp"
    "Recording.h"
    "SceneUpdater.cpp"
    "SceneUpdater.h"
)

target_include_directories(Camelot_replay PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(Camelot_replay
    PUBLIC
        Avalon_geometry
        Avalon_renderer
        Avalon_ui
        Camelot_data
        glm::glm
    PRIVATE
        spdlog::spdlog
)
