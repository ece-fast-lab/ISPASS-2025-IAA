#!/bin/bash

# Exit the script if any command fails
set -e

GIT_ROOT=$(git rev-parse --show-toplevel)
cd "$GIT_ROOT"

# Step 1: Clone the QPL repository with all submodules
echo "Cloning the QPL repository..."
git clone --recursive https://github.com/intel/qpl.git

# Step 2: Move into the qpl directory
cd qpl

# Step 3: Create a build directory
echo "Creating build directory..."
mkdir build
cd build

# Step 4: Run CMake to configure the build
echo "Configuring the build with CMake..."
echo "current path: $(pwd)"
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(pwd) ..

# Step 5: Build and install QPL
echo "Building and installing QPL..."
cmake --build . --target install

echo "QPL library installation completed!"
