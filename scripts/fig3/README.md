# 📊 Figure 3: Speedups of functional pipelining

## Steps to Generate and View Figure 3

### 1️⃣ Make the Script Executable
```bash
chmod +x run_fig3.sh
```

### 2️⃣ Run the Script
```bash
./run_fig3.sh -p hardware_path -d [/PATH_TO/DATA] -e 8 -s auto
```
Example:
```bash
./run_fig3.sh -p hardware_path -d /PATH_TO/ISPASS-2025-IAA/data/silesia_data/FILE_NAME -e 8 -s auto
```

### 3️⃣ Draw the Figure
```bash
python3 draw_fig3.py
```

### 4️⃣ View the Generated Figures
Check the output image files:
```
fig3.png
```

---

## 📌 Description
This script generates and visualizes the results of the **speedups achieved through functional pipelining**, comparing non-pipelined and pipelined performance

---

### **Explanation of Flags**
| Flag | Description |
|------|------------|
| **`-p`** | Specifies the execution path. Choose between: `software_path` (CPU) or `hardware_path` (IAA) or all (running both). *Note: Pipelined functionality does not support `software_path`.* |
| **`-d`** | Path to the input data file for running microbenchmarks. |
| **`-e`** | Number of engines to use (valid range: `1-8`) if all is selected in -p no need to select |
| **`-s`** | Chunk size for each job descriptor. Accepts: an integer from `1` to `2*1024*1024`, or `auto`. If `auto` is used, the script determines an optimal chunk size based on file size and the number of engines. if all is selected in -p no need to select |
