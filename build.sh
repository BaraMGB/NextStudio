#!/bin/bash

echo "----------------------------------------------------------"
echo "Starting build script..."
# Ensure the script is running under Bash
if [ -z "$BASH_VERSION" ]; then
    echo "Error: This script needs to be run using Bash, but it seems to be running in a different shell."
    exit 1
fi

# Initialize clean cache variable
CLEAN_CACHE=0

# Check if the clean cache argument is provided
if [[ " $@ " =~ " -clean " ]]; then
    CLEAN_CACHE=1
fi

# Remove -clean from the arguments
BUILD_ARGS=$(echo "$@" | sed 's/-clean//g')

# Check if the build type argument is provided and valid
if [[ -z "$BUILD_ARGS" ]] || [[ "$BUILD_ARGS" != "d" && "$BUILD_ARGS" != "r" && "$BUILD_ARGS" != "rd" ]]; then
    echo "Error: No build type specified or invalid build type."
    echo "Usage: $0 [build type] [-clean]"
    echo "Build type:"
    echo "  d  - Debug"
    echo "  r  - Release"
    echo "  rd - RelWithDebInfo"
    exit 1
else
    echo "Build type provided: $BUILD_ARGS"
echo "----------------------------------------------------------"
fi

# Set the build type based on the argument
case $BUILD_ARGS in
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
BUILD_DIR=./build/$BUILD_TYPE
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Conditionally remove previous CMake cache
if [ $CLEAN_CACHE -eq 1 ]; then
    echo "Removing previous CMake cache..."
    echo "----------------------------------------------------------"
    rm -f CMakeCache.txt
else
    echo "Skipping CMake cache removal."
    echo "----------------------------------------------------------"
fi

# Configure CMake and build the project
echo "Configuring CMake and starting the build..."
echo "----------------------------------------------------------"
if cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../.. && make -j4; then
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

# Define the executable path based on BUILD_TYPE
EXECUTABLE_PATH="./build/$BUILD_TYPE/App/NextStudio_artefacts/$BUILD_TYPE/NextStudio"

# Create start.sh script
echo "Creating start.sh script..."
echo "----------------------------------------------------------"

cat << EOF > start.sh

#!/bin/bash
echo "Starting the application..."

if [[ " \$@ " =~ " -d " ]]; then
    if [[ "\$OSTYPE" == "darwin"* ]]; then
        echo "MacOS.."
        if command -v lldb >/dev/null; then
            echo "Running with lldb..."
            lldb -o run $EXECUTABLE_PATH.app
        else
            echo "Warning: lldb is not installed. Unable to run the program with lldb."
        fi
    else
        echo "Linux.."
        if command -v gdb >/dev/null; then
            echo "Running with gdb..."
            gdb -ex=r --args $EXECUTABLE_PATH
        else
            echo "Warning: gdb is not installed. Unable to run the program with gdb."
        fi
    fi
else
    echo "Running without gdb..."
    if [[ "\$OSTYPE" == "darwin"* ]]; then
        open $EXECUTABLE_PATH.app
    else
        $EXECUTABLE_PATH
    fi
fi


EOF

# Make start.sh executable
echo "Making start.sh executable..."
echo "----------------------------------------------------------"
chmod +x start.sh

echo "Build script completed."
echo "----------------------------------------------------------"

