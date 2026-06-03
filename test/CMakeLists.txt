cmake_minimum_required(VERSION 3.14)
project(nebuladb_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Locate project root ──────────────────────────────────────────────────────
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# ── Output all test artifacts to test/build/ ─────────────────────────────────
set(OUTPUT_DIR ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CASE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
# 添加三方库
add_subdirectory(third_party)

# 添加源码编译
add_subdirectory(${PROJECT_ROOT}/src ${CMAKE_BINARY_DIR}/src)

if (${TEST} STREQUAL "UT_COMMON")
    add_subdirectory(dt/ut/common)
endif()
# add_subdirectory(dt/ut/compiler)
# add_subdirectory(dt/ut/query)
# add_subdirectory(dt/ut/executor)
# add_subdirectory(dt/ut/storage)
# add_subdirectory(dt/ut/service)
# add_subdirectory(dt/st)
# add_subdirectory(sdv)
