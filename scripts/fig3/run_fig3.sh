#!/bin/bash

# Exit if any command fails
set -e

# Get the Git root directory dynamically
GIT_ROOT=$(git rev-parse --show-toplevel)
MICRO_BENCHMARK_DIR="$GIT_ROOT/src/micro_benchmark"
SUMMARY_FILE="$GIT_ROOT/scripts/fig3/chaining_results.txt"

# Function to print usage
usage() {
    echo "Usage: $0 -p <hardware_path/software_path/all> -d <path_to_data> [-e <1-8>] [-s <0-2097152|auto>]"
    echo "  - If -p all is set, -e and -s are not required and -s is automatically set to auto."
    exit 1
}

# Parse and validate arguments
while getopts "p:d:e:s:" opt; do
    case "$opt" in
        p)
            if [[ "$OPTARG" != "hardware_path" && "$OPTARG" != "software_path" && "$OPTARG" != "all" ]]; then
                echo "Error: -p must be 'hardware_path', 'software_path', or 'all'."
                usage
            fi
            P_ARG="$OPTARG"
            ;;
        d)
            if [[ ! -f "$OPTARG" ]]; then
                echo "Error: -d must be a valid file path."
                usage
            fi
            D_ARG="$OPTARG"
            ;;
        e)
            if [[ "$P_ARG" != "all" ]]; then
                if [[ ! "$OPTARG" =~ ^[1-8]$ ]]; then
                    echo "Error: -e must be an integer between 1 and 8."
                    usage
                fi
                E_ARG="$OPTARG"
            fi
            ;;
        s)
            if [[ "$P_ARG" != "all" ]]; then
                if [[ "$OPTARG" == "auto" ]]; then
                    AUTO_CHUNK_SIZE=true
                elif [[ "$OPTARG" =~ ^[0-9]+$ && "$OPTARG" -ge 0 && "$OPTARG" -le $((2*1024*1024)) ]]; then
                    S_ARG="$OPTARG"
                    AUTO_CHUNK_SIZE=false
                else
                    echo "Error: -s must be an integer (0 to 2097152) or 'auto'."
                    usage
                fi
            fi
            ;;
        *)
            usage
            ;;
    esac
done

# Ensure required arguments are provided
if [[ -z "$P_ARG" || -z "$D_ARG" ]]; then
    usage
fi

# If -p all, set -s to auto and skip -e validation
if [[ "$P_ARG" == "all" ]]; then
    AUTO_CHUNK_SIZE=true
    unset E_ARG  # Ensure E_ARG is unset so it doesn't interfere
fi

# If -s is 'auto', calculate chunk size
if [[ "$AUTO_CHUNK_SIZE" == true ]]; then
    FILE_SIZE=$(stat -c %s "$D_ARG")  # Get file size in bytes
    CHUNK_SIZE=$(( FILE_SIZE / "$E_ARG" ))  # Default split into 4 chunks

    while (( CHUNK_SIZE + 1 > 2*1024*1024 )); do
        CHUNK_SIZE=$(( CHUNK_SIZE / 2 + 1 ))
    done
    
    S_ARG=$(( CHUNK_SIZE + 1 ))
fi

echo "Final chunk size (S_ARG): $S_ARG"

# Clear or create the summary file
echo "Microbenchmark Test Summary - $(date)" > "$SUMMARY_FILE"
echo "=====================================" >> "$SUMMARY_FILE"

# Navigate to micro_benchmark directory
cd "$MICRO_BENCHMARK_DIR"

# Function to run the benchmark
run_benchmark() {
    local P_VALUE="$1"
    local E_VALUES=(1 2 4 8)  # Used when -p all is set

    if [[ -n "$E_ARG" ]]; then
        # If E_ARG is explicitly set, use only that value
        E_VALUES=("$E_ARG")
    fi

    for DIR in */; do
        if [[ -d "$DIR" ]]; then
            for E_ARG in "${E_VALUES[@]}"; do
                if [[ "$DIR" != decompression* ]]; then
                    continue
                fi

                echo "Directory: $DIR | P_ARG: $P_VALUE | E_ARG: $E_ARG"
                cd "$DIR"

                # Ensure build.sh is executable
                if [[ ! -x "build.sh" ]]; then
                    chmod +x build.sh
                fi

                # Run build script
                ./build.sh

                # Detect the compiled program (exclude build.sh)
                PROGRAM_NAME=$(find . -maxdepth 1 -type f -executable ! -name "build.sh" -printf "%f\n" | head -n 1)

                if [[ -z "$PROGRAM_NAME" ]]; then
                    echo "Error: No executable found in $DIR after build."
                    exit 1
                fi

                # Log execution details
                echo "==========================================================================" >> "$SUMMARY_FILE"
                echo "Running ./$PROGRAM_NAME $P_VALUE $D_ARG $E_ARG $S_ARG" | tee -a "$SUMMARY_FILE"
                
                # Run the detected program and store the output
                ./"$PROGRAM_NAME" "$P_VALUE" "$D_ARG" "$E_ARG" "$S_ARG" | tee -a "$SUMMARY_FILE"
                echo "==========================================================================" >> "$SUMMARY_FILE"

                # Return to the micro_benchmark directory
                cd ..
            done
        fi
    done
}


# Handle different values of -p
echo "Running benchmarks for $P_ARG..."
run_benchmark "$P_ARG"

if [[ -d "$GIT_ROOT/data/silesia_data" && -d "$GIT_ROOT/data/silesia_data/compressed_data" && -d "$GIT_ROOT/data/silesia_data/decompressed_data" ]]; then
    rm -f "$GIT_ROOT/data/silesia_data/compressed_data/"*.iaa.*
    rm -f "$GIT_ROOT/data/silesia_data/decompressed_data/"*.iaa.*
fi

if [[ -d "$GIT_ROOT/data/clickbench_data" && -d "$GIT_ROOT/data/clickbench_data/compressed_data" && -d "$GIT_ROOT/data/clickbench_data/decompressed_data" ]]; then
    find "$GIT_ROOT/data/clickbench_data/compressed_data/" -type f -delete
    find "$GIT_ROOT/data/clickbench_data/decompressed_data/" -type f -delete
fi

echo "All benchmarks completed successfully!"
