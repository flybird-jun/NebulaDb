aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_COMMON)
add_library(common OBJECT ${SRC_COMMON})
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
