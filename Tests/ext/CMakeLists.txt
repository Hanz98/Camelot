add_subdirectory(icd)



if(WIN32)
    configure_file(
      ${CMAKE_SOURCE_DIR}/Tests/ext/icd/windows/VkICD_mock_icd.json
      ${CMAKE_BINARY_DIR}/lib/VkICD_mock_icd.json
      COPYONLY
    )
elseif(APPLE)
    configure_file(
      ${CMAKE_SOURCE_DIR}/Tests/ext/icd/macos/VkICD_mock_icd.json
      ${CMAKE_BINARY_DIR}/lib/VkICD_mock_icd.json
      COPYONLY
    )
elseif(UNIX AND NOT APPLE) # i.e. Linux
configure_file(
      ${CMAKE_SOURCE_DIR}/Tests/ext/icd/linux/VkICD_mock_icd.json
      ${CMAKE_BINARY_DIR}/lib/VkICD_mock_icd.json
      COPYONLY
    )
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()
