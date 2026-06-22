#!/bin/bash
set -euo pipefail

# Parse arguments (viel einfacher jetzt)
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <d|r|rd> [-clean]"
    echo "  d   Debug"
    echo "  r   Release"
    echo "  rd  RelWithDebInfo"
    echo "  -clean  (optional) Clear CMake cache before building"
    exit 1
fi

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

JOBS="${BUILD_JOBS:-2}"

echo "Building $BUILD_TYPE with $JOBS job(s)..."

# Create build directory
BUILD_DIR="./autobuild/$BUILD_TYPE"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Clean cache if requested
[[ "$CLEAN_CACHE" == true ]] && rm -rf CMakeCache.txt CMakeFiles/

# Build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ../..
cmake --build . -j "$JOBS"


# Move compile commands
[[ -f compile_commands.json ]] && mv compile_commands.json ../../

echo "Build completed successfully!"
