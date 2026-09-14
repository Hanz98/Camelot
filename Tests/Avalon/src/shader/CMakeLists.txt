target_sources(Test_Main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ShaderModuleTest.cpp
)

# Lets the test load a module from the .spv files in the build tree.
get_target_property(_shader_dir Avalon_shaders CAMELOT_SHADER_OUTPUT_DIR)
target_compile_definitions(Test_Main
    PRIVATE CAMELOT_TEST_SHADER_DIR="${_shader_dir}")
