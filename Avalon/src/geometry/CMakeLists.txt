add_library(Avalon_geometry
  "Shapes.cpp"
  "Shapes.h"
  "Vertex.cpp"
  "Vertex.h"
)

target_include_directories(Avalon_geometry PUBLIC ${Avalon_include_dirs})

target_link_libraries(Avalon_geometry
    PUBLIC
        Avalon
        glm::glm
)
