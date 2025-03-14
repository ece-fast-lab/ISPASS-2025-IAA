#!/bin/bash

# Get the Git root directory
GIT_ROOT=$(git rev-parse --show-toplevel)

# Define the QPL include and library paths relative to the Git root
QPL_INCLUDE="$GIT_ROOT/qpl/include"
QPL_LIB="$GIT_ROOT/qpl/build/lib/libqpl.a"

# Compile the program using the dynamically determined paths
g++ -I"$QPL_INCLUDE" -o expand_test expand_test.cpp "$QPL_LIB" -ldl
