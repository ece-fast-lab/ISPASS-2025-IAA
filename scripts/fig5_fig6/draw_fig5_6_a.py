import os
import argparse
import matplotlib.pyplot as plt
import numpy as np

# Set up argument parser
parser = argparse.ArgumentParser(description="Parse and normalize timing information from .txt files and store it in bar charts")
parser.add_argument("input_dir", type=str, help="Input directory")
args = parser.parse_args()

# Define lists for compression and decompression suffixes
suffices_compression = ["nation", "region", "supplier", "customer", "part", "partsupp", "orders", "lineitem"]
decompression_list = list(range(1, 23))

# Define prefixes in desired order: ZSTD first
prefices_decompression = ["zstd_", "lz4_", "pglz_", "qpl_"]
logics = ["compression", "decompression"]

# Define colors for bar chart
colors = {
    "zstd_": "#001f3f",  # Dark navy (ZSTD)
    "lz4_": "#003366",   # Lighter navy (LZ4)
    "pglz_": "#336699",  # Lighter than LZ4 (PGLZ)
    "qpl_": "#6699CC"    # Lightest (QPL)
}

# Create a dictionary to store data for each logic
excel_data = {logic: {} for logic in logics}

for logic in logics:
    if logic == "compression":
        for prefix in prefices_decompression:
            for suffix in suffices_compression:
                fname = os.path.join(args.input_dir, f"src/end_to_end/postgresql_citus/timing_output/tpc-h/{logic}_logic/{prefix}{suffix}.txt")
                if os.path.exists(fname):
                    with open(fname, 'r') as f:
                        for line in f:
                            line = line.strip()
                            if "Init Time" in line:
                                excel_data[logic].setdefault(prefix, {})[suffix] = [float(line.split()[2])]
                            elif "Wait Time" in line:
                                tmp = excel_data[logic][prefix][suffix]
                                excel_data[logic][prefix][suffix].append(float(line.split()[2]))
                            elif "Init Time" not in line and "Time:" in line:
                                tmp = excel_data[logic][prefix][suffix]
                                exe_time = float(line.split()[1])
                                excel_data[logic][prefix][suffix].append(exe_time)
                                break
    else:
        for prefix in prefices_decompression:
            for suffix in ["query"]:
                for i in decompression_list:
                    fname = os.path.join(args.input_dir, f"src/end_to_end/postgresql_citus/timing_output/tpc-h/{logic}_logic/{prefix}{suffix}{i}.txt")                    
                    if os.path.exists(fname):
                        with open(fname, 'r') as f:
                            for line in f:
                                line = line.strip()
                                if "Init Time" in line:
                                    excel_data[logic].setdefault(prefix, {})[f"Q{i}"] = [float(line.split()[2])]
                                elif "Wait Time" in line:
                                    tmp = excel_data[logic][prefix][f"Q{i}"]
                                    excel_data[logic][prefix][f"Q{i}"].append(float(line.split()[2]))
                                elif "Init Time" not in line and "Time:" in line:
                                    tmp = excel_data[logic][prefix][f"Q{i}"]
                                    exe_time = float(line.split()[1])
                                    excel_data[logic][prefix][f"Q{i}"].append(exe_time)
                                    break
                    else:
                        excel_data[logic].setdefault(prefix, {})[f"Q{i}"] = ["ERROR"]

# Normalize based on ZSTD values
for logic in logics:
    for key in excel_data[logic].get("zstd_", {}):  # Ensure key exists in ZSTD
        zstd_value = excel_data[logic]["zstd_"].get(key, [1])[-1]  # Default to 1 if missing
        if isinstance(zstd_value, (int, float)) and zstd_value > 0:
            for prefix in prefices_decompression:
                if prefix in excel_data[logic] and key in excel_data[logic][prefix]:
                    original_value = excel_data[logic][prefix][key][-1]
                    if isinstance(original_value, (int, float)):  # Avoid errors with non-numeric values
                        excel_data[logic][prefix][key][-1] = original_value / zstd_value

# ===========================
# Plot Bar Graphs
# ===========================

def plot_graph(logic, sorted_x_labels, filename, figsize):
    """Generates bar plots for compression and decompression results with ZSTD first."""
    x = np.arange(len(sorted_x_labels))  # Label positions on x-axis
    width = 0.2  # Bar width

    fig, ax = plt.subplots(figsize=figsize)

    for i, prefix in enumerate(prefices_decompression):  # Ensure ZSTD is always first
        values = [excel_data[logic].get(prefix, {}).get(label, [0])[-1] for label in sorted_x_labels]
        ax.bar(x + (i - 1.5) * width, values, width, label=prefix.rstrip('_').upper(), color=colors[prefix])

    ax.set_xlabel("Tables" if logic == "compression" else "Queries")
    ax.set_ylabel("Normalized Elapsed Time")
    ax.set_title(f"{logic.capitalize()} Performance (Normalized to ZSTD)")
    ax.set_xticks(x)
    ax.set_xticklabels(sorted_x_labels, ha="center")
    ax.legend()

    plt.tight_layout()
    plt.savefig(filename)
    plt.close()

# Plot Compression Graph (Normal Width)
compression_x_labels = ["nation", "region", "supplier", "customer", "part", "partsupp", "orders", "lineitem"]
plot_graph("compression", compression_x_labels, "fig5_a.png", figsize=(10, 6))

# Plot Decompression Graph (Double Width)
decompression_x_labels = [f"Q{i}" for i in range(1, 23)]
plot_graph("decompression", decompression_x_labels, "fig6_a.png", figsize=(20, 6))
