#!/bin/bash

# Check if the build type argument is provided and valid
if [[ $# -eq 0 ]] || [[ "$1" != "d" && "$1" != "r" && "$1" != "rd" ]]; then
    echo "Usage: $0 [build type]"
    echo "Build type:"
    echo "  d  - Debug"
    echo "  r  - Release"
    echo "  rd - RelWithDebInfo"
    exit 1
fi

# Set the build type based on the argument
case $1 in
    d)
        BUILD_TYPE="Debug"
        ;;
    r)
        BUILD_TYPE="Release"
        ;;
    rd)
        BUILD_TYPE="RelWithDebInfo"
        ;;
esac

# Create and enter the build directory
mkdir -p ./build/
cd build

# Remove previous CMake cache
rm -f CMakeCache.txt

# Configure CMake and build the project
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. && make -j6

# Move compile_commands.json if it exists
if [ -f ~/Projects/NextStudio/builds/compile_commands.json ]; then
    mv ~/Projects/NextStudio/builds/compile_commands.json ~/Projects/NextStudio/
fi

# Go back to the original directory
cd ..

# Create start.sh script
cat << EOF > start.sh
#!/bin/bash
./build/App/NextStudio_artefacts/$BUILD_TYPE/NextStudio
EOF

# Make start.sh executable
chmod +x start.sh
