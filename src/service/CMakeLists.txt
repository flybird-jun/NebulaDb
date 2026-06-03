# ── service module (OBJECT library) ──────────────────────────────────────────
add_library(nebuladb_service OBJECT
    src/placeholder.cpp
)

target_include_directories(nebuladb_service PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
