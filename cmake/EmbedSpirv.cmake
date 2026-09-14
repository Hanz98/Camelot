# Converts a SPIR-V binary into a C++ translation unit that defines the module
# as an array of 32-bit words. Invoked at build time by camelot_add_shaders():
#
#   cmake -DINPUT=<file.spv> -DOUTPUT=<file.cpp> -DSYMBOL=<identifier>
#         -DNAMESPACE=<ns> -P EmbedSpirv.cmake
#
# The words are emitted in the file's own byte order (detected from the magic
# number), so the array compares equal to what ShaderModule::readSpirv() reads
# back from the same file on a little-endian host.

foreach(var INPUT OUTPUT SYMBOL NAMESPACE)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "EmbedSpirv.cmake: ${var} is not set")
  endif()
endforeach()

file(READ "${INPUT}" _hex HEX)
string(LENGTH "${_hex}" _hex_len)
math(EXPR _bytes "${_hex_len} / 2")
math(EXPR _remainder "${_bytes} % 4")
if(_bytes EQUAL 0 OR NOT _remainder EQUAL 0)
  message(FATAL_ERROR "${INPUT}: size ${_bytes} is not a multiple of 4 bytes; not a SPIR-V module")
endif()
math(EXPR _words "${_bytes} / 4")

# SPIR-V magic number is 0x07230203; the first four bytes tell us the file's
# endianness so the words can be reassembled correctly.
string(SUBSTRING "${_hex}" 0 8 _magic)
if(_magic STREQUAL "03022307")
  string(REGEX REPLACE "(..)(..)(..)(..)" "0x\\4\\3\\2\\1u," _body "${_hex}")
elseif(_magic STREQUAL "07230203")
  string(REGEX REPLACE "(..)(..)(..)(..)" "0x\\1\\2\\3\\4u," _body "${_hex}")
else()
  message(FATAL_ERROR "${INPUT}: bad SPIR-V magic number (${_magic})")
endif()

# Eight words per line keeps the generated file readable and well below any
# compiler's line-length limit.
string(REPEAT "0x[0-9a-f]+u," 8 _line_pattern)
string(REGEX REPLACE "(${_line_pattern})" "\\1\n    " _body "${_body}")
string(REGEX REPLACE "[ \n]+$" "" _body "${_body}")

get_filename_component(_input_name "${INPUT}" NAME)
file(WRITE "${OUTPUT}" "\
// Generated from ${_input_name} by cmake/EmbedSpirv.cmake. Do not edit.
#include <cstddef>
#include <cstdint>

namespace ${NAMESPACE}::detail {

extern const uint32_t ${SYMBOL}_words[] = {
    ${_body}
};
extern const std::size_t ${SYMBOL}_word_count = ${_words};

}  // namespace ${NAMESPACE}::detail
")
