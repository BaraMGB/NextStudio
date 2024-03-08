#!/bin/bash

echo "----------------------------------------------------------"
echo "Starting build script..."
# Ensure the script is running under Bash
if [ -z "$BASH_VERSION" ]; then
    echo "Error: This script needs to be run using Bash, but it seems to be running in a different shell."
    exit 1
fi

# Check if the build type argument is provided and valid
if [[ $# -eq 0 ]] || [[ "$1" != "d" && "$1" != "r" && "$1" != "rd" ]]; then
    echo "Error: No build type specified or invalid build type."
    echo "Usage: $0 [build type]"
    echo "Build type:"
    echo "  d  - Debug"
    echo "  r  - Release"
    echo "  rd - RelWithDebInfo"
    exit 1
else
    echo "Build type provided: $1"
echo "----------------------------------------------------------"
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

echo "Configured build type: $BUILD_TYPE"
echo "----------------------------------------------------------"

# Create and enter the build directory
echo "Creating and entering the build directory..."
echo "----------------------------------------------------------"
            # mkdir -p ./build/
            # cd build || exit
BUILD_DIR=./build/$BUILD_TYPE
mkdir -p $BUILD_DIR
cd $BUILD_DIR
# Remove previous CMake cache
echo "Removing previous CMake cache..."
echo "----------------------------------------------------------"
rm -f CMakeCache.txt

# Configure CMake and build the project
echo "Configuring CMake and starting the build..."
echo "----------------------------------------------------------"
if cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../.. && make -j6; then
    echo "Build succeeded."
    echo "----------------------------------------------------------"
else
    echo "Build failed."
    echo "----------------------------------------------------------"
    exit 1
fi

# Move compile_commands.json if it exists
if [ -f compile_commands.json ]; then
    echo "Moving compile_commands.json to the project root..."
    echo "----------------------------------------------------------"
    mv compile_commands.json ../../
else
    echo "compile_commands.json does not exist, skipping."
    echo "----------------------------------------------------------"
fi

# Go back to the original directory
cd ../.. || exit

# Create start.sh script
echo "Creating start.sh script..."
echo "----------------------------------------------------------"

cat << EOF > start.sh
#!/bin/bash
echo "Starting the application..."
if command -v gdb >/dev/null; then
    # gdb ist installiert, führe das Programm mit gdb aus
    gdb -ex=r --args ./build/$BUILD_TYPE/App/NextStudio_artefacts/$BUILD_TYPE/NextStudio
else
    # gdb ist nicht installiert, gebe eine Warnung aus und führe das Programm ohne gdb aus
    echo "Warning: gdb is not installed. Running the program without it."
    ./build/$BUILD_TYPE/App/NextStudio_artefacts/$BUILD_TYPE/NextStudio
fi
EOF

# Make start.sh executable
echo "Making start.sh executable..."
echo "----------------------------------------------------------"
chmod +x start.sh

echo "Build script completed."
echo "----------------------------------------------------------"
