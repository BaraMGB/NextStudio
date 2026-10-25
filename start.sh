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
    echo "  -clean    Remove the build directory for a fresh build"
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
CLEAN_BUILD=false
BUILD_ONLY=false
RUN_WITH_DEBUGGER=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        d|r|rd) BUILD_TYPE="$1" ;;
        -clean) CLEAN_BUILD=true ;;
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
BUILD_ARGS=("$BUILD_TYPE")
[[ "$CLEAN_BUILD" == true ]] && BUILD_ARGS+=("-clean")

if ! ./build.sh "${BUILD_ARGS[@]}"; then
    echo "Build failed!"
    exit 1
fi

# Run if not build-only
if [[ "$BUILD_ONLY" == false ]]; then
    echo "=== Starting application ==="
    
    # Determine executable path. JUCE adds the configuration directory for
    # both single- and multi-configuration generators.
    case $BUILD_TYPE in
        d) CONFIGURATION="Debug" ;;
        r) CONFIGURATION="Release" ;;
        rd) CONFIGURATION="RelWithDebInfo" ;;
    esac

    ARTEFACT_PATH="./autobuild/${CONFIGURATION}/App/NextStudio_artefacts/${CONFIGURATION}/NextStudio"
    APP_PATH=""

    if [[ "${OSTYPE:-}" == "darwin"* ]]; then
        APP_PATH="${ARTEFACT_PATH}.app"
        EXEC_PATH="${APP_PATH}/Contents/MacOS/NextStudio"
    elif [[ "${OSTYPE:-}" == msys* || "${OSTYPE:-}" == cygwin* || "${OSTYPE:-}" == win32* ]]; then
        EXEC_PATH="${ARTEFACT_PATH}.exe"
    else
        EXEC_PATH="$ARTEFACT_PATH"
    fi

    if [[ ! -x "$EXEC_PATH" ]]; then
        echo "Executable not found: $EXEC_PATH" >&2
        exit 1
    fi

    # Run with or without debugger
    if [[ "$RUN_WITH_DEBUGGER" == true ]]; then
        if [[ "${OSTYPE:-}" == "darwin"* ]]; then
            lldb -o run -- "$EXEC_PATH"
        else
            gdb -ex=r --args "$EXEC_PATH"
        fi
    else
        if [[ "${OSTYPE:-}" == "darwin"* ]]; then
            open "$APP_PATH"
        else
            "$EXEC_PATH"
        fi
    fi
fi

