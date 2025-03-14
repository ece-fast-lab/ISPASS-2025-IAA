#!/bin/bash

# Ensure script stops if any command fails
set -e

# Arguments
PG_USER=$1

# Get Git repository root
GIT_REPO_DIR=$(git rev-parse --show-toplevel)
POSTGRESQL_DIR="$GIT_REPO_DIR/postgresql-15.9"

# Compression methods
COMPRESSION_METHODS=("zstd" "lz4" "pglz" "qpl")

# Loop through each compression method
for COMPRESSION_METHOD in "${COMPRESSION_METHODS[@]}"; do
    echo "Running experiments for compression method: $COMPRESSION_METHOD"

    # Generate SQL tables
    python3 "$GIT_REPO_DIR/scripts/fig5_fig6/generate_sql_table.py" \
        "$GIT_REPO_DIR/scripts/fig5_fig6/table_a.sql" \
        "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/tpch_sql/table/$COMPRESSION_METHOD/" \
        "$COMPRESSION_METHOD" \
        "$GIT_REPO_DIR/data/tpc_h_data"

    # ================================
    # Restart PostgreSQL
    # ================================
    $POSTGRESQL_DIR/bin/pg_ctl -D $POSTGRESQL_DIR/data -l $POSTGRESQL_DIR/server.log restart

    # ================================
    # Run Cleanup SQL File
    # ================================
    psql -U $PG_USER -d template1 -f "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/tpch_sql/tpc-h_clean_table.sql"

    $POSTGRESQL_DIR/bin/pg_ctl -D $POSTGRESQL_DIR/data -l $POSTGRESQL_DIR/server.log restart

    # Create necessary directories
    mkdir -p "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/"
    mkdir -p "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/compression_logic/"
    mkdir -p "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/decompression_logic/"

    # ================================
    # Run SQL Scripts for Compression
    # ================================
    TABLES=("nation" "region" "supplier" "customer" "part" "partsupp" "orders" "lineitem")

    OUTPUT_FILE="$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/${COMPRESSION_METHOD}_compression.txt"

    for TABLE in "${TABLES[@]}"; do
        SQL_FILE="$GIT_REPO_DIR/src/end_to_end/postgresql_citus/tpch_sql/table/$COMPRESSION_METHOD/$TABLE.sql"
        LOG_FILE="$POSTGRESQL_DIR/data/qpl_debug_log/postgresql_test.log"
        FINAL_LOG="$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/compression_logic/${COMPRESSION_METHOD}_${TABLE}.txt"

        echo "Executing compression for $TABLE using $COMPRESSION_METHOD..."
        psql -U $PG_USER -d template1 -f "$SQL_FILE" >> "$OUTPUT_FILE"

        mv "$LOG_FILE" "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/compression_logic/${COMPRESSION_METHOD}.txt"
        python3 "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/tpch_sql/logic_timing_parse.py" \
            "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/compression_logic/${COMPRESSION_METHOD}.txt" \
            "$FINAL_LOG"

        rm -f "$LOG_FILE"
        $POSTGRESQL_DIR/bin/pg_ctl -D $POSTGRESQL_DIR/data -l $POSTGRESQL_DIR/server.log restart
    done

    # COMPRESSION_OUTPUT_FILE="$GIT_REPO_DIR/postgresql_${COMPRESSION_METHOD}_compression.txt"
    # python3 "$GIT_REPO_DIR/compression_result_parse.py" "$OUTPUT_FILE" "$COMPRESSION_OUTPUT_FILE"

    # ================================
    # Run Queries for Decompression
    # ================================
    QUERIES=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22)
    OUTPUT_FILE="$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/${COMPRESSION_METHOD}_decompression.txt"

    for QUERY in "${QUERIES[@]}"; do
        QUERY_FILE="$GIT_REPO_DIR/src/end_to_end/postgresql_citus/tpch_sql/query/query${QUERY}.sql"
        LOG_FILE="$POSTGRESQL_DIR/data/qpl_debug_log/postgresql_test.log"
        FINAL_LOG="$GIT_REPO_DIR/src/end_to_end/postgresql_citus/timing_output/tpc-h/decompression_logic/${COMPRESSION_METHOD}_query${QUERY}.txt"

        echo "Executing decompression for query $QUERY using $COMPRESSION_METHOD..."
        psql -U $PG_USER -d template1 -f "$QUERY_FILE" >> "$OUTPUT_FILE"

        mv "$LOG_FILE" "$FINAL_LOG"
        python3 "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/tpch_sql/logic_timing_parse.py" "$FINAL_LOG" "$FINAL_LOG"

        rm -f "$LOG_FILE"
        $POSTGRESQL_DIR/bin/pg_ctl -D $POSTGRESQL_DIR/data -l $POSTGRESQL_DIR/server.log restart
    done

    # ================================
    # Final Cleanup & Timing Processing
    # ================================
    # DECOMPRESSION_OUTPUT_FILE="$GIT_REPO_DIR/postgresql_${COMPRESSION_METHOD}_decompression.txt"
    # python3 "$GIT_REPO_DIR/decompression_result_parse.py" "$OUTPUT_FILE" "$DECOMPRESSION_OUTPUT_FILE"
    rm -f "$POSTGRESQL_DIR/data/qpl_debug_log/postgresql_test.log"

    psql -U $PG_USER -d template1 -f "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/tpch_sql/tpc-h_clean_table.sql"
done

python3 draw_fig5_6_a.py "$GIT_REPO_DIR/"
