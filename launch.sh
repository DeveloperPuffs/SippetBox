#!/bin/bash

# This shell script is used for building and running the editor or specific examples.
# Build an example: `./launch.sh debug examples/your_example`
# Build the editor: `./launch.sh debug editor`

# Save the original directory and move the directory of this script
ORIGINAL_DIRECTORY=$(pwd)
cd "$(dirname "$0")"

# Detect build type for CMake
BUILD_TYPE="$(echo "${1:0:1}" | tr '[:lower:]' '[:upper:]')${1:1}"
if [[ "$BUILD_TYPE" != "Debug" ]] && [[ "$BUILD_TYPE" != "Release" ]]; then
  echo "Unrecognized build type: $BUILD_TYPE"
  echo "Usage: $0 <debug|release> path/to/build/target [clean]"
  exit 1
fi

echo "Build type: $BUILD_TYPE"

# Check if a valid build target is provided 
TARGET=$2
if [[ -z "$TARGET" ]]; then
  echo "Unrecognized build target: $TARGET"
  echo "Usage: $0 <debug|release> path/to/build/target [clean]"
  exit 1
fi

echo "Build target: $TARGET"

# Check whether the clean folder should be cleaned
if [[ "$(echo "$2" | tr '[:upper:]' '[:lower:]')" == "clean" ]]; then
  echo "Cleaning build folder..."
  rm -rf build
fi

# Move to the build folder
mkdir -p build
cd build

# Configure and build CMake
echo "Configuring and building CMake ..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DLAUNCH_TARGET=$TARGET ..
cmake --build .

# Move back to the original directory
cd "$ORIGINAL_DIRECTORY"

# Determine the name of the executable based on the target directory and then run it
EXECUTABLE=$(basename "$TARGET")
if [[ "$TARGET" == "editor" ]]; then
  echo "Running SippetBox ..."
  ./build/editor/SippetBox
elif [[ -d "build/$TARGET" && -f "build/$TARGET/$EXECUTABLE" ]]; then
  echo "Running example: $TARGET ..."
  ./build/$TARGET/$EXECUTABLE
else
  echo "Error: Executable for '$TARGET' not found"
  exit 1
fi