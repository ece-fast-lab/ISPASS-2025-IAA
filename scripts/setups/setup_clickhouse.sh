#!/bin/bash

# Exit on error
set -e

# Get the Git repository root
GIT_REPO_DIR=$(git rev-parse --show-toplevel)

# Navigate to the repository root
cd "$GIT_REPO_DIR"

# Clone the ClickHouse repository recursively
git clone --recursive https://github.com/ClickHouse/ClickHouse.git

# Navigate into the ClickHouse directory
cd ClickHouse

# Checkout the specified version
git checkout 24.8

# Update submodules
git submodule update --init --recursive

# Copy custom CMakeLists.txt
cp "$GIT_REPO_DIR/src/end_to_end/clickhouse/clickhouse.cpp" "$GIT_REPO_DIR/ClickHouse/src/Compression/CompressionCodecDeflateQpl.cpp"

# Configure the build with CMake
cmake -D CMAKE_C_COMPILER=/usr/bin/clang-18 \
      -D CMAKE_CXX_COMPILER=/usr/bin/clang++-18 \
      -D ENABLE_QPL=1 \
      -S . -B build

# Build ClickHouse
cmake --build build --target clickhouse
