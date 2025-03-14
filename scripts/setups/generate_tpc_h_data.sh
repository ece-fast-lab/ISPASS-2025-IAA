#!/bin/bash

# Exit if any command fails
set -e

echo "Generating TPC_H data..."

GIT_ROOT=$(git rev-parse --show-toplevel)
cd "$GIT_ROOT"

# Step 1: Move to the tpch_gentool directory
cd data/tpch_gentool
echo "Moved to $(pwd)"

# Step 2: Unzip gentool_tpc_h.zip
unzip gentool_tpc_h.zip
echo "Unzipped gentool_tpc_h.zip"

# Step 3: Move to 'TPC-H V3.0.1' directory
cd "TPC-H V3.0.1"
echo "Moved to $(pwd)"

# Step 4: Move to the dbgen directory
cd dbgen
echo "Moved to $(pwd)"

# Step 5: Update makefile.suite
cp ../../update_makefile.suite makefile.suite
echo "Updated makefile.suite"

# Step 6: Compile dbgen
make -f makefile.suite
echo "Compilation complete"

# Step 7: Run dbgen command
./dbgen -s 1 -v
echo "dbgen execution complete"

# Step 8: Move generated .tbl files to tpc_h_data
mv *.tbl ../../../tpc_h_data
cd ../../../tpc_h_data
echo "Moved .tbl files to $(pwd)"

# Step 9: Run organize.sh
chmod +x organize.sh  # Ensure script is executable
./organize.sh
echo "Data organization complete"

echo "Generated TPC_H data successfully!"
