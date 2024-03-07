#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <build_type>"
    echo "build_type: d for Debug, r for Release, rd for RelWithDebInfo"
    exit 1
fi

case $1 in
    d) BUILD_TYPE="Debug" ;;
    r) BUILD_TYPE="Release" ;;
    rd) BUILD_TYPE="RelWithDebInfo" ;;
    *)
        echo "Invalid build type: $1"
        echo "Valid types are: d (Debug), r (Release), rd (RelWithDebInfo)"
        exit 1
        ;;
esac

mkdir -p ./build/
cd build
rm -f CMakeCache.txt
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. && make -j6 && mv ~/Projects/NextSTudio/build/compile_commands.json ~/Projects/NextSTudio/ && ./App/NextStudio_artefacts/$BUILD_TYPE/NextStudio
cd ..

