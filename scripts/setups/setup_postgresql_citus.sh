#!/bin/bash

# Exit on any error
set -e

# Parse arguments
THREADS=1  # Default number of threads
while getopts ":t:" opt; do
  case ${opt} in
    t ) THREADS=$OPTARG ;;
    \? ) echo "Usage: $0 [-t threads]"; exit 1 ;;
  esac
done

# Define GIT_REPO_DIR
GIT_REPO_DIR=$(git rev-parse --show-toplevel)

# Step 1: Change to GIT_REPO_DIR
cd "$GIT_REPO_DIR"

# Step 2: Download PostgreSQL source code
wget https://ftp.postgresql.org/pub/source/v15.9/postgresql-15.9.tar.gz

# Step 3: Extract PostgreSQL source
tar -xvzf postgresql-15.9.tar.gz

# Step 4: Change to PostgreSQL directory
cd postgresql-15.9/

# Step 5: Configure PostgreSQL
./configure --prefix="$GIT_REPO_DIR/postgresql-15.9"

# Step 6: Compile PostgreSQL using the specified number of threads
make world -j"$THREADS"

# Step 7: Install PostgreSQL
make install-world

# Step 8: Initialize PostgreSQL database
"$GIT_REPO_DIR/postgresql-15.9/bin/initdb" -D "$GIT_REPO_DIR/postgresql-15.9/data"

# Step 9: Start PostgreSQL
"$GIT_REPO_DIR/postgresql-15.9/bin/pg_ctl" -D "$GIT_REPO_DIR/postgresql-15.9/data" -l "$GIT_REPO_DIR/postgresql-15.9/server.log" start

# Step 10: Return to GIT_REPO_DIR
cd "$GIT_REPO_DIR"

# Step 11: Clone Citus repository
git clone https://github.com/citusdata/citus.git

# Step 12: Change to Citus directory
cd citus

# Step 13: Apply the patch
git apply "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/citus_iaa_setup.patch"

# Step 14: Update Makefile with correct QPL paths
MAKEFILE="$GIT_REPO_DIR/citus/src/backend/columnar/Makefile"
sed -i "s|/PATH_TO/qpl/include/qpl|$GIT_REPO_DIR/qpl/include/qpl|g" "$MAKEFILE"
sed -i "s|/PATH_TO/qpl/build/lib|$GIT_REPO_DIR/qpl/build/lib|g" "$MAKEFILE"

# Step 15: Set PG_CONFIG environment variable
export PG_CONFIG="$GIT_REPO_DIR/postgresql-15.9/bin/pg_config"

# Step 16: Configure Citus
./configure

# Step 17: Compile and install Citus
make clean && make -j"$THREADS" && make install

# Step 18: Update PostgreSQL configuration
cp "$GIT_REPO_DIR/src/end_to_end/postgresql_citus/postgresql.conf" "$GIT_REPO_DIR/postgresql-15.9/data/postgresql.conf"

# Step 19: Restart PostgreSQL
"$GIT_REPO_DIR/postgresql-15.9/bin/pg_ctl" -D "$GIT_REPO_DIR/postgresql-15.9/data" -l "$GIT_REPO_DIR/postgresql-15.9/server.log" restart

echo "PostgreSQL setup completed successfully!"
