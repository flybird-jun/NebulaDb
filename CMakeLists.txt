cmake_minimum_required(VERSION 3.14)
project(NebulaDb VERSION 1.0.0 LANGUAGES CXX)

# ── C++17 ────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Output directories ───────────────────────────────────────────────────────
# All build artifacts (so / a / exe) go to <project>/output/
set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# ── Compiler flags ───────────────────────────────────────────────────────────
add_compile_options(-Wall -Wextra -Werror)

MESSAGE(WARNING "go here")
# ── Source modules ───────────────────────────────────────────────────────────
add_subdirectory(src)

add_library(nebuladb SHARED  $<TARGET_OBJECTS:common>)

target_link_libraries(nebuladb PRIVATE common)