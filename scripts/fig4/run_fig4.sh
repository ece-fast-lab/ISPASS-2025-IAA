#!/bin/bash

# Exit script if any command fails
set -e

# Get the base directory of the Git repo dynamically
GIT_ROOT=$(git rev-parse --show-toplevel)
SUMMARY_FILE="$GIT_ROOT/scripts/fig4/pandas_summary.txt"

# Move to pandas evaluation directory
cd "$GIT_ROOT/src/end_to_end/pandas"

# Run split_lineitem.py
python3 split_lineitem.py "$GIT_ROOT/data/tpc_h_data/lineitem.tbl" "$GIT_ROOT/data/lineitem/" >"$SUMMARY_FILE"

# Ensure iaa_preprocess.sh is executable
if [[ ! -x "iaa_preprocess.sh" ]]; then
    chmod +x iaa_preprocess.sh
fi

# Run iaa_preprocess.sh
./iaa_preprocess.sh

# Run iaa_lineitem_compression with all .tbl files
mkdir -p "$GIT_ROOT/data/lineitem/iaa_compressed/"
for tbl_file in "$GIT_ROOT/data/lineitem/"*.bin; do
    ./iaa_lineitem_compression hardware_path "$GIT_ROOT/data/lineitem/" "$(basename "$tbl_file")" "$GIT_ROOT/data/lineitem/iaa_compressed/" 2 > "$SUMMARY_FILE"
done
echo "End-End (Pandas) Test Summary - $(date)" > "$SUMMARY_FILE"
echo "=====================================" >> "$SUMMARY_FILE"

# Run cpu_based_q6.py with different compression methods
for method in lz4 snappy zlib zstd; do
    echo "==========================================================================" >> "$SUMMARY_FILE"
    python3 cpu_based_q6.py "$GIT_ROOT/data/lineitem/" "$method" | tee -a "$SUMMARY_FILE"
done

# Ensure iaa_q6.sh is executable
if [[ ! -x "iaa_q6.sh" ]]; then
    chmod +x iaa_q6.sh
fi

# Run iaa_q6.sh
./iaa_q6.sh

SE_VALUES=(1)  # Removed 'local'
for SE_ARG in "${SE_VALUES[@]}"; do
    echo "==========================================================================" >> "$SUMMARY_FILE"
    ./decompression_scan software_path "$GIT_ROOT/data/lineitem/iaa_compressed/" "$SE_ARG" "$GIT_ROOT/data/lineitem/" | tee -a "$SUMMARY_FILE"
done
# Run decompression_scan with provided -e argument
E_VALUES=(1 2 4 8)  # Removed 'local'
for E_ARG in "${E_VALUES[@]}"; do
    echo "==========================================================================" >> "$SUMMARY_FILE"
    echo "IAA with $E_ARG engines" >> "$SUMMARY_FILE"
    ./decompression_scan hardware_path "$GIT_ROOT/data/lineitem/iaa_compressed/" "$E_ARG" "$GIT_ROOT/data/lineitem/" | tee -a "$SUMMARY_FILE"
done

echo "Pandas evaluation completed successfully!"
