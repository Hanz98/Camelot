target_sources(Test_Main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/MainModelTest.cpp
)
target_link_libraries(Test_Main PRIVATE Camelot_main)
