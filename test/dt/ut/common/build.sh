#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TEST_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_DIR="$TEST_ROOT/build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$TEST_ROOT" -DCMAKE_BUILD_TYPE=Debug -DCASE_DIR="$SCRIPT_DIR" -DTEST="UT_COMMON"
make -j"$(nproc)"
