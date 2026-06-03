# ── compiler module (OBJECT library) ─────────────────────────────────────────
add_library(nebuladb_compiler OBJECT
    src/placeholder.cpp
)

target_include_directories(nebuladb_compiler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
