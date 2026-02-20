#!/bin/bash
set -euo pipefail

show_usage() {
    echo "Usage: $0 [build_type] [options]"
    echo "Build types:"
    echo "  d   - Debug"
    echo "  r   - Release" 
    echo "  rd  - RelWithDebInfo"
    echo ""
    echo "Options:"
    echo "  -clean    Clean CMake cache before build"
    echo "  -build    Only build, don't run"
    echo "  -debug    Run with debugger (gdb/lldb)"
    echo ""
    echo "Examples:"
    echo "  ./start.sh d -clean     # Clean build debug and run"
    echo "  ./start.sh r -build     # Build release only"
    echo "  ./start.sh d -debug     # Build debug and run with debugger"
}

# Default values
BUILD_TYPE=""
CLEAN_CACHE=false
BUILD_ONLY=false
RUN_WITH_DEBUGGER=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        d|r|rd) BUILD_TYPE="$1" ;;
        -clean) CLEAN_CACHE=true ;;
        -build) BUILD_ONLY=true ;;
        -debug) RUN_WITH_DEBUGGER=true ;;
        -h|--help) show_usage; exit 0 ;;
        *) echo "Unknown option: $1"; show_usage; exit 1 ;;
    esac
    shift
done

# Validate build type
if [[ -z "$BUILD_TYPE" ]]; then
    echo "Error: Build type required"
    show_usage
    exit 1
fi

# Build the project
echo "=== Building project ==="
BUILD_ARGS="$BUILD_TYPE"
[[ "$CLEAN_CACHE" == true ]] && BUILD_ARGS="$BUILD_ARGS -clean"

if ! ./build.sh $BUILD_ARGS; then
    echo "Build failed!"
    exit 1
fi

# Run if not build-only
if [[ "$BUILD_ONLY" == false ]]; then
    echo "=== Starting application ==="
    
    # Determine executable path
    case $BUILD_TYPE in
        d) EXEC_PATH="./autobuild/Debug/App/NextStudio_artefacts/Debug/NextStudio" ;;
        r) EXEC_PATH="./autobuild/Release/App/NextStudio_artefacts/Release/NextStudio" ;;
        rd) EXEC_PATH="./autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio" ;;
    esac
    
    # Run with or without debugger
    if [[ "$RUN_WITH_DEBUGGER" == true ]]; then
        if [[ "$OSTYPE" == "darwin"* ]]; then
            lldb -o run "$EXEC_PATH.app"
        else
            gdb -ex=r --args "$EXEC_PATH"
        fi
    else
        if [[ "$OSTYPE" == "darwin"* ]]; then
            open "$EXEC_PATH.app"
        else
            "$EXEC_PATH"
        fi
    fi
fi

