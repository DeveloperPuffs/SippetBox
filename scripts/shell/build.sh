# This shell script will configure and build the project using CMake.

ORIGINAL_DIRECTORY=$(pwd)
cd "$(dirname "$0")/../.."

mkdir -p build
cd build

cmake ..
cmake --build .

cd "$ORIGINAL_DIRECTORY"