# 📊 Figure 2: Microbenchmark Results

## Steps to Generate and View Figure 2

### 1️⃣ Make the Script Executable
```bash
chmod +x run_fig2.sh
```

### 2️⃣ Run the Script
```bash
./run_fig2.sh -p all -d [/PATH_TO/DATA]
```
Example:
```bash
./run_fig2.sh -p all -d /PATH_TO/ISPASS-2025-IAA/data/silesia_data/FILE_NAME
```
**⚠️Important Note**: Before running the microbenchmark with `hits.csv`, ensure that there is sufficient memory (more than 80GB) available. If there is not enough memory, the script may not provide accurate performance results.

### 3️⃣ Draw the Figure
```bash
python3 draw_fig2.py
```

### 4️⃣ View the Generated Figures
Check the output image files:
```
fig2_a.png
fig2_b.png
fig2_c.png
fig2_d.png
fig2_e.png
fig2_f.png
fig2_g.png
fig2_h.png
```
**⚠️Important Note**: If the results from the software path appear incorrect, please run it multiple times. Due to thread synchronization, the software path does not guarantee consistent trends.

---

## 📌 Description
This script generates and visualizes the results of the **microbenchmark functions supported by IAA**, comparing **CPU vs. IAA** performance. It evaluates performance across:
- **Threads:** 1, 2, 4, 8
- **IAA Engines:** 1, 2, 4, 8

---

### **Explanation of Flags**
| Flag | Description |
|------|------------|
| **`-p`** | Specifies the execution path. Choose between: `software_path` (CPU) or `hardware_path` (IAA) or all (running both). *Note: Pipelined functionality does not support `software_path`.* |
| **`-d`** | Path to the input data file for running microbenchmarks. |
| **`-e`** | Number of engines to use (valid range: `1-8`) if all is selected in -p no need to select |
| **`-s`** | Chunk size for each job descriptor. Accepts: an integer from `1` to `2*1024*1024`, or `auto`. If `auto` is used, the script determines an optimal chunk size based on file size and the number of engines. if all is selected in -p no need to select |
