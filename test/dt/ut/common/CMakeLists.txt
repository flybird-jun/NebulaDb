aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_UT_COMMON)
add_executable(ut_common ${SRC_UT_COMMON})
target_link_libraries(ut_common PRIVATE common GTest::gtest GTest::gtest_main)
