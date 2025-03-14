#!/bin/bash

# Exit on error
set -e

# Get the Git repository root
GIT_REPO_DIR=$(git rev-parse --show-toplevel)
SCRIPT_DIR="$GIT_REPO_DIR/scripts/fig7_fig8"
CLICKHOUSE_DIR="$GIT_REPO_DIR/ClickHouse/build/programs"
CLICKHOUSE_CLIENT="$CLICKHOUSE_DIR/clickhouse client"
BASE_DIR="$GIT_REPO_DIR/src/end_to_end/clickhouse/tpch_sql"
QUERY_DIR="${BASE_DIR}/query"

# Define compression methods
COMPRESSION_METHODS=("LZ4" "ZSTD" "DEFLATE_QPL")

# Loop through each compression method
for COMPRESSION_METHOD in "${COMPRESSION_METHODS[@]}"; do
    echo "Processing compression method: $COMPRESSION_METHOD"

    # Create a new SQL file with the modified content
    sed "s/LZ4/$COMPRESSION_METHOD/g; s|/PATH_TO/|$GIT_REPO_DIR/|g" \
        "$SCRIPT_DIR/table_b.sql" > "$BASE_DIR/create_${COMPRESSION_METHOD}_table.sql"

    # Create output directories
    mkdir -p "${BASE_DIR}/output/compression"
    mkdir -p "${BASE_DIR}/output/decompression"

    # Drop existing tables
    $CLICKHOUSE_CLIENT --time --queries-file="${BASE_DIR}/drop_table.sql"

    # Create new tables
    $CLICKHOUSE_CLIENT --time --queries-file="${BASE_DIR}/create_${COMPRESSION_METHOD}_table.sql" \
        > "${BASE_DIR}/output/compression/${COMPRESSION_METHOD}.txt" 2>&1

    # Run queries 1 to 10
    for i in {1..10}; do
        QUERY_FILE="${QUERY_DIR}/query${i}.sql"
        if [ -f "$QUERY_FILE" ]; then
            echo "Running $QUERY_FILE..."
            $CLICKHOUSE_CLIENT --time --queries-file="$QUERY_FILE" \
                > "${BASE_DIR}/output/decompression/${COMPRESSION_METHOD}_query${i}.txt" 2>&1
        else
            echo "Warning: $QUERY_FILE does not exist."
        fi
    done

    # Drop tables after execution
    $CLICKHOUSE_CLIENT --time --queries-file="${BASE_DIR}/drop_table.sql"
done

python3 draw_fig7_8_b.py "$GIT_REPO_DIR/src/end_to_end/clickhouse/tpch_sql/output/"

echo "ClickHouse experiment script completed successfully!"

