# :gear: System Setup Guide

## :pushpin: Install QPL Library
Ensure your system meets the requirements outlined in the **[QPL Installation Guide](https://intel.github.io/qpl/documentation/get_started_docs/installation.html)**.

1. Make the script executable:
   ```bash
   chmod +x generate_qpl.sh
   ```
2. Run the script:
   ```bash
   ./generate_qpl.sh
   ```
3. After execution, the QPL library will be installed in:
   ```
   ISPASS-2025-IAA/qpl
   ```

---

## :pushpin: Enable IAA
1. Make the script executable:
   ```bash
   chmod +x configure_iaa_user.sh
   ```
2. Run the script:
   ```bash
   sudo ./configure_iaa_user.sh
   ```
3. After execution, **eight engines and one group** will be configured.

4. To verify the configuration, run:
   ```bash
   accel-config list
   ```
5. Ensure to change the ownership of `/dev/iax` to your user:
   ```bash
   sudo chown -R [username] /dev/iax
   ```
   - If the owner is **root**, then running the code **with sudo** will not provide proper results for **microbenchmarks** and **end-to-end applications**.

---

## :books: To Generate Dataset

### :pushpin: Generating **TPC-H 1GB Dataset**
1. Make the script executable:
   ```bash
   chmod +x generate_tpc_h_data.sh
   ```
2. Run the script:
   ```bash
   ./generate_tpc_h_data.sh
   ```
3. After execution, the dataset will be created in:
   ```
   ISPASS-2025-IAA/data/tpc_h_data
   ```

---

### :pushpin: Downloading **Silesia Corpus & Clickbench**
- **Silesia Corpus**  
  1. Download **12 files** from the [Silesia Corpus website](https://sun.aei.polsl.pl//~sdeor/index.php?page=silesia).  
  2. Place them in the following directory after building the directory:
     ```
     ISPASS-2025-IAA/data/silesia_data
     ```

- **Clickbench (hits.csv)**
  1. Download `hits.csv.gz` from [Clickbench](https://github.com/ClickHouse/ClickBench).
  2. Place it in after building the directory:
     ```
     ISPASS-2025-IAA/data/clickbench_data
     ```
  3. Unzip the file (if compressed) using:
     ```bash
     gzip -d hits.csv.gz
     ```
---

## :books: Database Setup

###  **Check for Running PostgreSQL Instances**
Before proceeding, ensure that **no other PostgreSQL instance is running**.

Check the active processes:
```bash
ps aux | grep postgres
```
If PostgreSQL is running, stop it using:
```bash
systemctl stop postgresql   # For system-wide service
```
or stop it locally if running as a user process.

---

### :pushpin: PostgreSQL & Citus Setup
Ensure that no other postgresql is running, check it with ps aux | grep postgres. If postgresql it is running then run systemctl stop postgresql to stop or stop it locally
1. Make the script executable:
   ```bash
   chmod +x setup_postgresql_citus.sh
   ```
2. Run the script:
Replace [NUMBER_OF_THREADS] with the number of threads you want to use:
   ```bash
   ./setup_postgresql_citus.sh -t [NUMBER_OF_THREADS]
   ```
Example: To use 16 threads:
   ```bash
   ./setup_postgresql_citus.sh -t 16
   ```
3. Create Citus Extension:
   ```
   psql template1
   CREATE EXTENSION citus;
   \q
   ```

---

### :pushpin: Clichouse Setup
1. Make the script executable:
   ```bash
   chmod +x setup_clickhouse.sh
   ```
2. Run the script:
   ```bash
   ./setup_clickhouse.sh
   ```
3. Check if ClickHouse has been successfully built by verifying the existence of the following directory:
   ```bash
   ls /PATH_TO/ISPASS-2025-IAA/ClickHouse
   ```
4. To start the ClickHouse server, navigate to the build directory and run the server:
   ```bash
   cd /PATH_TO/ISPASS-2025-IAA/ClickHouse/build/programs
   ./clickhouse server
   ```
   Leave this terminal open while running ClickHouse.
5. Open a new terminal and run experiments:
- Check **Figure 7 & Figure 8** in the documentation:
  **[Figure 7 & Figure 8](../fig7_fig8/README.md)**.


