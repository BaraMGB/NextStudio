#!/bin/bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <d|r|rd>"
    echo "  d   Debug"
    echo "  r   Release"
    echo "  rd  RelWithDebInfo"
    exit 1
fi

case "$1" in
    d)  BUILD_DIR="Debug" ;;
    r)  BUILD_DIR="Release" ;;
    rd) BUILD_DIR="RelWithDebInfo" ;;
    *)
        echo "Invalid build type: $1"
        exit 1
        ;;
esac

./build.sh "$1"
ctest --test-dir "autobuild/${BUILD_DIR}" --output-on-failure
