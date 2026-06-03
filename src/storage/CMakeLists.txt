# ── storage module (OBJECT library) ──────────────────────────────────────────
add_library(nebuladb_storage OBJECT
    src/placeholder.cpp
)

target_include_directories(nebuladb_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
