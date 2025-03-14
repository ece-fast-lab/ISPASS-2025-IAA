import os
import argparse
import matplotlib.pyplot as plt
import numpy as np

# Set up argument parser
parser = argparse.ArgumentParser(description="Parse and normalize timing information for compression and decompression")
parser.add_argument("input_dir", type=str, help="Input directory")
args = parser.parse_args()

# Define compression methods in the desired order: ZSTD first
compression_methods = ["ZSTD", "LZ4", "DEFLATE_QPL"]

# Define colors for each compression method
colors = {
    "ZSTD": "#001f3f",  # Dark navy (ZSTD)
    "LZ4": "#003366",   # Lighter navy (LZ4)
    "DEFLATE_QPL": "#6699CC"  # Lightest (DEFLATE_QPL)
}

# X-axis labels
compression_x_labels = ["nation", "region", "supplier", "customer", "part", "partsupp", "orders", "lineitem"]
decompression_x_labels = [f"Q{i}" for i in range(1, 11)]  # Q1 to Q10

# Dictionary to store parsed data
compression_data = {method: [] for method in compression_methods}
decompression_data = {method: [] for method in compression_methods}

# ===========================
# Compression Data Extraction
# ===========================
for method in compression_methods:
    file_path = os.path.join(args.input_dir, "compression", f"{method}.txt")
    if os.path.exists(file_path):
        with open(file_path, "r") as f:
            lines = f.readlines()

        # Extract every 4th line float value
        extracted_values = []
        for i in range(3, len(lines), 4):  # Start from index 3 (4th line) and take every 4th
            try:
                extracted_values.append(float(lines[i].strip()))
            except ValueError:
                continue

        # Store values, ensuring alignment with x-labels
        compression_data[method] = extracted_values[:len(compression_x_labels)]

# ===========================
# Decompression Data Extraction
# ===========================
for method in compression_methods:
    extracted_values = []
    for i in range(1, 11):  # Query1 to Query10
        file_path = os.path.join(args.input_dir, "decompression", f"{method}_query{i}.txt")
        if os.path.exists(file_path):
            with open(file_path, "r") as f:
                lines = f.readlines()

            if lines:
                try:
                    extracted_values.append(float(lines[-1].strip()))  # Get last line float
                except ValueError:
                    continue

    # Store values, ensuring alignment with x-labels
    decompression_data[method] = extracted_values[:len(decompression_x_labels)]

# ===========================
# Normalize Data Based on ZSTD
# ===========================
def normalize_data(data_dict, x_labels):
    """Normalize elapsed times based on ZSTD values."""
    if "ZSTD" in data_dict and len(data_dict["ZSTD"]) == len(x_labels):  # Ensure ZSTD has full data
        zstd_values = np.array(data_dict["ZSTD"])
        normalized_data = {method: np.array(values) / zstd_values for method, values in data_dict.items()}
        return normalized_data
    return data_dict  # If no ZSTD data, return original

normalized_compression = normalize_data(compression_data, compression_x_labels)
normalized_decompression = normalize_data(decompression_data, decompression_x_labels)

# ===========================
# Plot Bar Graphs
# ===========================
def plot_graph(data, x_labels, title, filename):
    """Generates a bar chart with specified x_labels, ensuring ZSTD is always first."""
    
    fig, ax = plt.subplots(figsize=(10, 6))
    x = np.arange(len(x_labels))  # X-axis positions
    width = 0.25  # Bar width

    # Ensure ZSTD is always first, followed by LZ4 and DEFLATE_QPL
    for i, method in enumerate(compression_methods):  
        values = data.get(method, [0] * len(x_labels))  # Default to zeros if missing
        ax.bar(x + (i - 1) * width, values, width, label=method, color=colors[method])

    ax.set_ylabel("Normalized Elapsed Time")
    ax.set_title(title)
    ax.set_xticks(x)
    ax.set_xticklabels(x_labels, ha="center")
    ax.legend()

    plt.tight_layout()
    plt.savefig(filename)
    plt.close()

# Generate compression and decompression graphs with correct x-labels
plot_graph(normalized_compression, compression_x_labels, "Compression Performance (Normalized to ZSTD)", "fig7_b.png")
plot_graph(normalized_decompression, decompression_x_labels, "Decompression Performance (Normalized to ZSTD)", "fig8_b.png")

print("Compression and decompression graphs saved!")
