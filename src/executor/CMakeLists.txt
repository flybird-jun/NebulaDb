# ── executor module (OBJECT library) ─────────────────────────────────────────
add_library(nebuladb_executor OBJECT
    src/placeholder.cpp
)

target_include_directories(nebuladb_executor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
