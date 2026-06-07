#!/bin/bash
# Usage:
#   sh build.sh          configure & build
#   sh build.sh clean    remove the build/ directory

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

case "${1:-build}" in
    clean)
        echo "Removing $BUILD_DIR ..."
        rm -rf "$BUILD_DIR"
        echo "Done."
        ;;
    build|"")
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake .. && make -j32
        ;;
    *)
        echo "Unknown command: $1" >&2
        echo "Usage: $0 [build|clean]" >&2
        exit 1
        ;;
esac
