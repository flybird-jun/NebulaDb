# ── query module (OBJECT library) ────────────────────────────────────────────
add_library(nebuladb_query OBJECT
    src/placeholder.cpp
)

target_include_directories(nebuladb_query PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
