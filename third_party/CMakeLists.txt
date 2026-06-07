
include(ExternalProject)
# 设置三方库相关变量
set(THIRD_PARTY_BUILD_DIR   ${CMAKE_BINARY_DIR}/third_party/build)
set(THIRD_PARTY_OUTPUT_DIR ${CMAKE_BINARY_DIR}/third_party/output)
set(THIRD_PARTY_INCLUDE_DIR ${CMAKE_BINARY_DIR}/third_party/include)

# jemalloc库相关
set(JEMALLOC_BUILD_DIR ${THIRD_PARTY_BUILD_DIR}/jemalloc)
set(JEMALLOC_OUTPUT_DIR ${THIRD_PARTY_OUTPUT_DIR}/jemalloc)
set(JEMALLOC_INCLUDE_DIR ${THIRD_PARTY_INCLUDE_DIR}/jemalloc)

ExternalProject_Add(
    jemalloc_external
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jemalloc"
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --prefix=${JEMALLOC_BUILD_DIR}
        --libdir=${JEMALLOC_OUTPUT_DIR}
        --includedir=${JEMALLOC_INCLUDE_DIR}
        --disable-cxx
        CFLAGS=-fPIC
    BUILD_COMMAND make -j32
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE ON
    BUILD_BYPRODUCTS "${JEMALLOC_OUTPUT_DIR}/libjemalloc.a"   # 或 .so，按需指定
)

add_library(jemalloc INTERFACE)
target_include_directories(jemalloc INTERFACE "${JEMALLOC_INCLUDE_DIR}")
target_link_libraries(jemalloc INTERFACE "${JEMALLOC_OUTPUT_DIR}/libjemalloc.a")
add_dependencies(jemalloc jemalloc_external)
