#!/bin/bash
set -euo pipefail

# Parse arguments (viel einfacher jetzt)
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <d|r|rd> [-clean]"
    echo "  d   Debug"
    echo "  r   Release"
    echo "  rd  RelWithDebInfo"
    echo "  -clean  (optional) Remove the build directory for a fresh build"
    exit 1
fi

BUILD_TYPE_ARG="$1"
CLEAN_BUILD=false

[[ "${2:-}" == "-clean" ]] && CLEAN_BUILD=true

# Map build types
case $BUILD_TYPE_ARG in
    d) BUILD_TYPE="Debug" ;;
    r) BUILD_TYPE="Release" ;;
    rd) BUILD_TYPE="RelWithDebInfo" ;;
    *) echo "Invalid build type: $BUILD_TYPE_ARG"; exit 1 ;;
esac

JOBS="${BUILD_JOBS:-2}"

echo "Building $BUILD_TYPE with $JOBS job(s)..."

# Build directory
BUILD_DIR="./autobuild/$BUILD_TYPE"

# Remove the build directory for a fresh build if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    echo "Removing $BUILD_DIR for a clean build..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Build. --config is required by multi-configuration generators such as
# Visual Studio; single-configuration generators accept it as well.
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ../..
cmake --build . --config "$BUILD_TYPE" --parallel "$JOBS"


# Move compile commands
[[ -f compile_commands.json ]] && mv compile_commands.json ../../

echo "Build completed successfully!"
