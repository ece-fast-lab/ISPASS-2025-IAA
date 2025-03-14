# Intel® In-Memory Analytics Accelerator (IAA): Performance Characterization and Guidelines

This section provides guidelines for reproducing the key experimental results presented in the paper. The experiments are categorized into three main sets:
(1) evaluations of the throughput for each function supported by IAA, as well as the speedup achieved through IAA’s pipelined functionality compared to executing functions without pipelining on specific benchmarks (Fig. 2 and Fig. 3);
(2) assessments of decompression performance and TPC-H Query 6 using IAA, alongside alternative approaches such as Pandas combined with various compression algorithms (LZ4, SNAPPY, ZLIB, ZSTD, and DEFLATE) (Fig. 4);
(3) performance analysis of TPC-H queries within database applications, including PostgreSQL with Citus and ClickHouse (Fig. 5, Fig. 6, Fig. 7, Fig. 8).

## 📊 Reproducing Experimental Results

This section provides instructions to reproduce key experimental results shown in the paper. The experiments include:

1. **Function Throughput Evaluation** (Fig. 2) - Evaluates the throughput of functions supported by Intel IAA. Refer to **[scripts/fig2/README.md](scripts/fig2/README.md)**.
2. **Pipelined vs. Non-Pipelined Performance** (Fig. 3) - Compares IAA's pipelined functionality to non-pipelined functionality. **[scripts/fig3/README.md](scripts/fig3/README.md)**..
3. **Decompression & TPC-H Query 6 Performance** (Fig. 4) - Compares Intel IAA's decompression benefits against Pandas and other libraries (LZ4, SNAPPY, ZLIB, ZSTD, and DEFLATE). Refer to **[scripts/fig4/README.md](scripts/fig4/README.md)**.
4. **PostgreSQL with Citus Compression & Decompression** (Fig. 5 & Fig. 6) - Evaluates IAA's compression and decompression under different settings in PostgreSQL with Citus. Refer to **[scripts/fig5_fig6/README.md](scripts/fig5_fig6/README.md)**.
5. **ClickHouse Compression & Decompression** (Fig. 7 & Fig. 8) - Validates IAA's compression and decompression performance in ClickHouse under different settings. Refer to **[scripts/fig7_fig8/README.md](scripts/fig7_fig8/README.md)**.

---
## 🔩 Requirements

### **Hardware**
We have been tested on Intel Xeon Gold 6438Y+ CPU.  
- A server with an **Intel 4th-generation Xeon Scalable Processor** equipped with **IAA**.  

### **Software**
- Kernel 6.3.0+
- C++18 build toolchain
- Python 3.8+
- QPL version 1.6.0+

---
## 📖 Contents

- **`data/`** - Contains datasets for experiments.
- **`src/micro_benchmark/`** - Code for microbenchmarking IAA's functionalities and pipelined functionality.
- **`src/end_to_end/`** - Code for **Pandas, PostgreSQL with Citus, and ClickHouse** experiments.
- **`scripts/`** - Scripts for setting up the environment and generating Figures **2, 3, 4, 5, 6, 7, and 8**.

---
## 🚀 Experiment Workflow

1. **Setting Up the Environment**  
   Follow **`scripts/setups/README.md`** to set up the environment.

2. **Running Experiments**  
   Follow **`scripts/fig[i]/README.md`** to run experiments and generate **Figure i**.

---
## 📌 Final Notes

- Ensure **all datasets** are placed in the correct directories.
- Verify that **system requirements** are met before running benchmarks.
- Refer to **Intel QPL** or **IAA documentation** for troubleshooting related to IAA.

