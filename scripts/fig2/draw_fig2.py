import re
import matplotlib.pyplot as plt

# Define category mapping to filenames
category_filenames = {
    "Compression": "fig2_a.png",
    "Decompression": "fig2_b.png",
    "Scan": "fig2_c.png",
    "Scan Range": "fig2_d.png",
    "Extract": "fig2_e.png",
    "Select": "fig2_f.png",
    "Expand": "fig2_g.png",
    "CRC64": "fig2_h.png"
}

# Define x-axis labels
x_labels = ["CPU_1", "CPU_2", "CPU_4", "CPU_8", "IAA_1", "IAA_2", "IAA_4", "IAA_8"]

# Define custom colors
color_mapping = {
    "CPU_1": "#FF9999",  # Light Red
    "CPU_2": "#FFB266",  # Light Orange
    "CPU_4": "#FFDD57",  # Light Yellow
    "CPU_8": "#99FF99",  # Light Green
    "IAA_1": "#000080",  # Navy
    "IAA_2": "#0033A0",  # Brighter Navy than IAA_1
    "IAA_4": "#0066C0",  # Brighter Navy than IAA_2
    "IAA_8": "#3399FF"   # Brighter Navy than IAA_4
}

# Read the text file
with open("microbenchmark_results.txt", "r") as file:
    data = file.readlines()

# Initialize storage for results
parsed_results = {category: [] for category in category_filenames.keys()}

# Regex to capture category names and bandwidth values
category_pattern = re.compile(r"\[IAA (.*?)\]")  # Captures category names
bandwidth_pattern = re.compile(r"Bandwidth\s*=\s*([\d\.]+)\s*MB/s")  # Captures bandwidth values

# Track the current category while parsing
current_category = None

# Parse the text file
for line in data:
    category_match = category_pattern.search(line)
    if category_match:
        current_category = category_match.group(1)  # Extract category name
        if current_category == "Scan Exact":  # Adjust category name if needed
            current_category = "Scan"
    
    bandwidth_match = bandwidth_pattern.search(line)
    if bandwidth_match and current_category in parsed_results:
        parsed_results[current_category].append(float(bandwidth_match.group(1)) / 1000)  # Convert MB/s to GB/s

# Generate and save plots
for category, values in parsed_results.items():
    if len(values) == 8:  # Ensure we have exactly 8 values
        plt.figure(figsize=(8, 5))
        colors = [color_mapping[label] for label in x_labels]  # Assign colors based on x-labels
        plt.bar(x_labels, values, color=colors)
        plt.ylabel("Throughput (GB/s)")
        plt.title(f"Microbenchmark ({category})")
        plt.ylim(0, max(values) * 1.2)  # Adjust y-limit for better visualization
        plt.grid(axis="y", linestyle="--", alpha=0.7)

        # Save as specific filename based on category
        filename = category_filenames[category]
        plt.savefig(filename)
        plt.close()

print("Graphs have been successfully created with specified colors and filenames!")
