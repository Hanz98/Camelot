# camelot_add_shaders(<target>
#                     SOURCES <file>...
#                     [NAMESPACE <ns>]         default: <target>
#                     [HEADER <path>]          default: <target>/Registry.h
#                     [TARGET_ENV <env>]       default: vulkan1.1
#                     [OUTPUT_DIR <dir>])      default: ${CMAKE_BINARY_DIR}/shaders
#
# Compiles each GLSL source (stage taken from the extension: .vert, .frag,
# .comp, .geom, .tesc, .tese) to SPIR-V with glslangValidator, writes the
# result to OUTPUT_DIR/<name>.spv, and embeds it into a static library
# <target> whose generated header <HEADER> exposes the modules by source file
# name (see cmake/ShaderRegistry.h.in).
#
# Every shader has its own compile and embed step, so editing one GLSL file
# rebuilds only that module. A source that does not exist is a configure-time
# error. glslangValidator comes from the Conan tool requirement in
# conanfile.py (CMAKE_PROGRAM_PATH) or from a Vulkan SDK on PATH.
#
# Target properties set for consumers:
#   CAMELOT_SHADER_OUTPUT_DIR  directory holding the .spv files
#   CAMELOT_SHADER_NAMES       list of embedded shader names

find_program(CAMELOT_GLSLANG_VALIDATOR
  NAMES glslangValidator glslang
  DOC "GLSL to SPIR-V compiler used by camelot_add_shaders()")

set(_CAMELOT_SHADERS_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(camelot_add_shaders target)
  cmake_parse_arguments(ARG "" "NAMESPACE;HEADER;TARGET_ENV;OUTPUT_DIR" "SOURCES" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "camelot_add_shaders(${target}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "camelot_add_shaders(${target}): SOURCES is required")
  endif()
  if(NOT CAMELOT_GLSLANG_VALIDATOR)
    message(FATAL_ERROR
      "camelot_add_shaders(${target}): glslangValidator not found. "
      "Run `conan install` (it provides glslang) or put a Vulkan SDK on PATH.")
  endif()

  if(NOT ARG_NAMESPACE)
    set(ARG_NAMESPACE "${target}")
  endif()
  if(NOT ARG_HEADER)
    set(ARG_HEADER "${target}/Registry.h")
  endif()
  if(NOT ARG_TARGET_ENV)
    set(ARG_TARGET_ENV "vulkan1.1")
  endif()
  if(NOT ARG_OUTPUT_DIR)
    set(ARG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
  endif()

  set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/${target}_generated")
  set(include_dir "${gen_dir}/include")
  file(MAKE_DIRECTORY "${ARG_OUTPUT_DIR}" "${gen_dir}")

  set(embedded_sources "")
  set(extern_decls "")
  set(entries "")
  set(names "")
  set(spv_files "")

  foreach(source IN LISTS ARG_SOURCES)
    get_filename_component(abs_source "${source}" ABSOLUTE)
    if(NOT EXISTS "${abs_source}")
      message(FATAL_ERROR "camelot_add_shaders(${target}): shader source not found: ${source}")
    endif()

    get_filename_component(name "${abs_source}" NAME)
    get_filename_component(ext "${abs_source}" LAST_EXT)
    if(ext STREQUAL ".vert")
      set(stage VK_SHADER_STAGE_VERTEX_BIT)
    elseif(ext STREQUAL ".frag")
      set(stage VK_SHADER_STAGE_FRAGMENT_BIT)
    elseif(ext STREQUAL ".comp")
      set(stage VK_SHADER_STAGE_COMPUTE_BIT)
    elseif(ext STREQUAL ".geom")
      set(stage VK_SHADER_STAGE_GEOMETRY_BIT)
    elseif(ext STREQUAL ".tesc")
      set(stage VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
    elseif(ext STREQUAL ".tese")
      set(stage VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
    else()
      message(FATAL_ERROR
        "camelot_add_shaders(${target}): cannot infer the shader stage of ${source}; "
        "expected one of .vert .frag .comp .geom .tesc .tese")
    endif()
    if(name IN_LIST names)
      message(FATAL_ERROR "camelot_add_shaders(${target}): duplicate shader name ${name}")
    endif()
    list(APPEND names "${name}")

    string(MAKE_C_IDENTIFIER "${name}" symbol)
    set(spv "${ARG_OUTPUT_DIR}/${name}.spv")
    set(depfile "${gen_dir}/${name}.d")
    set(embedded "${gen_dir}/${symbol}.spv.cpp")

    # GLSL -> SPIR-V. The depfile covers #include'd files (GL_GOOGLE_include_directive).
    add_custom_command(
      OUTPUT "${spv}"
      COMMAND "${CAMELOT_GLSLANG_VALIDATOR}"
              -V --target-env "${ARG_TARGET_ENV}"
              --depfile "${depfile}"
              -o "${spv}" "${abs_source}"
      MAIN_DEPENDENCY "${abs_source}"
      DEPFILE "${depfile}"
      COMMENT "Compiling shader ${name}"
      VERBATIM)

    # SPIR-V -> C++ array.
    add_custom_command(
      OUTPUT "${embedded}"
      COMMAND "${CMAKE_COMMAND}"
              -DINPUT=${spv} -DOUTPUT=${embedded}
              -DSYMBOL=${symbol} -DNAMESPACE=${ARG_NAMESPACE}
              -P "${_CAMELOT_SHADERS_DIR}/EmbedSpirv.cmake"
      DEPENDS "${spv}" "${_CAMELOT_SHADERS_DIR}/EmbedSpirv.cmake"
      COMMENT "Embedding shader ${name}"
      VERBATIM)

    list(APPEND embedded_sources "${embedded}")
    list(APPEND spv_files "${spv}")
    string(APPEND extern_decls
      "extern const uint32_t ${symbol}_words[];\n"
      "extern const std::size_t ${symbol}_word_count;\n")
    string(APPEND entries
      "    ShaderBlob{\"${name}\", ${stage},\n"
      "               std::span<const uint32_t>(detail::${symbol}_words,\n"
      "                                         detail::${symbol}_word_count)},\n")
  endforeach()

  list(LENGTH names count)
  string(MAKE_C_IDENTIFIER "${ARG_HEADER}" guard)
  string(TOUPPER "${guard}_" guard)
  set(CAMELOT_SHADER_TARGET "${target}")
  set(CAMELOT_SHADER_NAMESPACE "${ARG_NAMESPACE}")
  set(CAMELOT_SHADER_HEADER "${ARG_HEADER}")
  set(CAMELOT_SHADER_GUARD "${guard}")
  set(CAMELOT_SHADER_COUNT "${count}")
  set(CAMELOT_SHADER_EXTERN_DECLS "${extern_decls}")
  set(CAMELOT_SHADER_ENTRIES "${entries}")
  configure_file("${_CAMELOT_SHADERS_DIR}/ShaderRegistry.h.in"
                 "${include_dir}/${ARG_HEADER}" @ONLY)
  configure_file("${_CAMELOT_SHADERS_DIR}/ShaderRegistry.cpp.in"
                 "${gen_dir}/${target}_registry.cpp" @ONLY)

  add_library(${target} STATIC
    "${gen_dir}/${target}_registry.cpp"
    ${embedded_sources})
  target_include_directories(${target} PUBLIC "${include_dir}")
  target_link_libraries(${target} PUBLIC Vulkan::Vulkan)
  target_compile_features(${target} PUBLIC cxx_std_20)
  set_target_properties(${target} PROPERTIES
    CAMELOT_SHADER_OUTPUT_DIR "${ARG_OUTPUT_DIR}"
    CAMELOT_SHADER_NAMES "${names}")

  # Lets `cmake --build . --target <target>_spv` produce just the .spv files.
  add_custom_target(${target}_spv DEPENDS ${spv_files})
endfunction()
