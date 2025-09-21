#!/bin/bash
set -euo pipefail

# Parse arguments (viel einfacher jetzt)
BUILD_TYPE_ARG="$1"
CLEAN_CACHE=false

[[ "${2:-}" == "-clean" ]] && CLEAN_CACHE=true

# Map build types
case $BUILD_TYPE_ARG in
    d) BUILD_TYPE="Debug" ;;
    r) BUILD_TYPE="Release" ;;
    rd) BUILD_TYPE="RelWithDebInfo" ;;
    *) echo "Invalid build type: $BUILD_TYPE_ARG"; exit 1 ;;
esac

echo "Building $BUILD_TYPE..."

# Create build directory
BUILD_DIR="./build/$BUILD_TYPE"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Clean cache if requested
[[ "$CLEAN_CACHE" == true ]] && rm -rf CMakeCache.txt CMakeFiles/

# Build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ../..
make -j4

# Move compile commands
[[ -f compile_commands.json ]] && mv compile_commands.json ../../

echo "Build completed successfully!"

