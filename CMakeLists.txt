cmake_minimum_required(VERSION 3.14)
project(NebulaDb VERSION 1.0.0 LANGUAGES CXX)

# ── C++20 ────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Output directories ───────────────────────────────────────────────────────
# All project build artifacts (so / a / exe) go to <project>/output/


add_compile_options(-Wall -Wextra -Werror -fPIC)

# 设置三方库相关变量
# set(THIRD_PARTY_BUILD_DIR   ${CMAKE_BINARY_DIR}/third_party/build/) # 设置三方库编译中间文件目录
# set(THIRD_PARTY_OUTPUT_DIR ${CMAKE_BINARY_DIR}/third_party/output/)
# set(THIRD_PARTY_INCLUDE_DIR ${CMAKE_BINARY_DIR}/third_party/include/)
add_subdirectory(third_party)
# include_directories(${THIRD_PARTY_INCLUDE_DIR})
# link_directories(${THIRD_PARTY_OUTPUT_DIR})

# 设置源码编译相关变量
set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
add_subdirectory(src)


add_library(nebuladb SHARED  $<TARGET_OBJECTS:common>)

target_link_libraries(nebuladb PRIVATE common)

# target_link_options(nebuladb PRIVATE
#     "LINKER:--whole-archive,$<TARGET_FILE:jemalloc>,--no-whole-archive"
# )
