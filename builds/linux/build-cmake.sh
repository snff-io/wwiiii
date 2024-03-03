#!/bin/sh

set -xe

# Navigate to the source directory
wwiv_source_root=$(find "$(pwd)" -name "WWIV_SOURCE_ROOT" 2>/dev/null | head -n 1) 
wwiv_source_root="$(dirname "$(readlink -f "$wwiv_source_root")")"
pushd "$wwiv_source_root"

# Clean previous build artifacts
rm -rf build
mkdir build
cd build

# Configure the project using CMake
cmake -S ../ -B ./ -DCMAKE_INSTALL_PREFIX:PATH=/opt/wwiv -DCMAKE_BUILD_TYPE:STRING=Debug 

#make with 8 jobs
make -j8

# Return to the previous directory
popd
