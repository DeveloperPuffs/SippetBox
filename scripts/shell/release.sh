# This shell script will configure and build using CMake with release build type, and then run the project.

ORIGINAL_DIRECTORY=$(pwd)
cd "$(dirname "$0")/../.."

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

cd "$ORIGINAL_DIRECTORY"
./"$(dirname "$0")/../../build/SippetBox"