import re
import numpy as np
import matplotlib.pyplot as plt

# Define the input text file
file_path = "pandas_summary.txt"

# Define category names and colors
categories = [
    "LZ4", "Snappy", "ZLIB", "ZSTD", "QPL",
    "Np_IAA_1", "Np_IAA_2", "Np_IAA_4", "Np_IAA_8", 
    "Np_IAA_1_LZ4", "Np_IAA_2_LZ4", "Np_IAA_4_LZ4", "Np_IAA_8_LZ4", 
    "P_IAA_1", "P_IAA_2", "P_IAA_4", "P_IAA_8"
]

colors = {
    "Decompression": "pink",
    "Scan": "lightblue",
    "Mask": "lightgreen",
    "Select": "lightsalmon",
    "Sum": "red"
}

# Initialize dictionary to store parsed data
parsed_results = {category: {"Decompression": 0, "Scan": 0, "Mask": 0, "Select": 0, "Sum": 0} for category in categories}

# Define regex patterns for extracting times
decompression_pattern = re.compile(r"Time taken for decompression:\s*([\d.]+)\s*seconds")
scan_pattern = re.compile(r"Time taken for q6 scan:\s*([\d.]+)\s*seconds")
select_pattern = re.compile(r"Time taken for q6 select:\s*([\d.]+)\s*seconds")
sum_pattern = re.compile(r"Time taken for q6 sum:\s*([\d.]+)\s*seconds")

iaa_decompress_pattern = re.compile(r"\(Decompress\) Time take:\s*([\d.]+)\s*sec")
iaa_scan_pattern = re.compile(r"\(Scan\) Time take:\s*([\d.]+)\s*sec")
iaa_mask_pattern = re.compile(r"\(Mask\) Time take:\s*([\d.]+)\s*sec")
iaa_select_pattern = re.compile(r"\(Select\) Time take:\s*([\d.]+)\s*sec")
iaa_sum_pattern = re.compile(r"\(Sum\) Time take:\s*([\d.]+)\s*sec")

# Read the file content
with open(file_path, "r") as file:
    content = file.readlines()

# Parsing state
category_index = 0  # Tracks LZ4, Snappy, ZLIB, ZSTD in order
iaa_index = 0  # Tracks Np_IAA_X and P_IAA_X in order
reading_iaa = False  # Flag to track IAA sections

# Iterate through file line by line
for line in content:
    # Parsing LZ4, Snappy, ZLIB, ZSTD
    if not reading_iaa and category_index < 4:
        if decompression_pattern.search(line):
            parsed_results[categories[category_index]]["Decompression"] = float(decompression_pattern.search(line).group(1))
        elif scan_pattern.search(line):
            parsed_results[categories[category_index]]["Scan"] = float(scan_pattern.search(line).group(1))
        elif select_pattern.search(line):
            parsed_results[categories[category_index]]["Select"] = float(select_pattern.search(line).group(1))
        elif sum_pattern.search(line):
            parsed_results[categories[category_index]]["Sum"] = float(sum_pattern.search(line).group(1))
            category_index += 1  # Move to next compression method after the full set
            
    # Detect QPL software path
    elif category_index == 4:
        if iaa_decompress_pattern.search(line):
            parsed_results[categories[category_index]]["Decompression"] = float(iaa_decompress_pattern.search(line).group(1))
        elif iaa_scan_pattern.search(line):
            parsed_results[categories[category_index]]["Scan"] = float(iaa_scan_pattern.search(line).group(1))
        elif iaa_mask_pattern.search(line):
            parsed_results[categories[category_index]]["Mask"] = float(iaa_mask_pattern.search(line).group(1))
        elif iaa_select_pattern.search(line):
            parsed_results[categories[category_index]]["Select"] = float(iaa_select_pattern.search(line).group(1))
        elif iaa_sum_pattern.search(line):
            parsed_results[categories[category_index]]["Sum"] = float(iaa_sum_pattern.search(line).group(1))
            category_index += 1  # Move to next compression method after the full set

    # Detect IAA with engines
    elif "IAA with" in line and "engines" in line:
        reading_iaa = True
        iaa_engine = int(re.search(r"IAA with (\d+) engines", line).group(1))
        iaa_category_np = f"Np_IAA_{iaa_engine}"
        iaa_category_p = f"P_IAA_{iaa_engine}"

    # Parsing Np_IAA_X values
    elif reading_iaa and iaa_index == 0:
        if iaa_decompress_pattern.search(line):
            parsed_results[iaa_category_np]["Decompression"] = float(iaa_decompress_pattern.search(line).group(1))
        elif iaa_scan_pattern.search(line):
            parsed_results[iaa_category_np]["Scan"] = float(iaa_scan_pattern.search(line).group(1))
        elif iaa_mask_pattern.search(line):
            parsed_results[iaa_category_np]["Mask"] = float(iaa_mask_pattern.search(line).group(1))
        elif iaa_select_pattern.search(line):
            parsed_results[iaa_category_np]["Select"] = float(iaa_select_pattern.search(line).group(1))
        elif iaa_sum_pattern.search(line):
            parsed_results[iaa_category_np]["Sum"] = float(iaa_sum_pattern.search(line).group(1))
            iaa_index = 1  # Move to next Np_IAA
        

    # Parsing P_IAA_X values (Decompression is always 0)
    elif reading_iaa and iaa_index == 1:
        if iaa_scan_pattern.search(line):
            parsed_results[iaa_category_p]["Scan"] = float(iaa_scan_pattern.search(line).group(1))
        elif iaa_mask_pattern.search(line):
            parsed_results[iaa_category_p]["Mask"] = float(iaa_mask_pattern.search(line).group(1))
        elif iaa_select_pattern.search(line):
            parsed_results[iaa_category_p]["Select"] = float(iaa_select_pattern.search(line).group(1))
        elif iaa_sum_pattern.search(line):
            parsed_results[iaa_category_p]["Sum"] = float(iaa_sum_pattern.search(line).group(1))
            parsed_results[iaa_category_p]["Decompression"] = 0  # Explicitly set decompression to 0
            reading_iaa = False  # Done parsing P_IAA_X
            iaa_index = 0
        

    # Create Np_IAA_X_LZ4 versions (using LZ4's scan, mask, select, sum)
    for i in [1, 2, 4, 8]:
        category_np_lz4 = f"Np_IAA_{i}_LZ4"
        parsed_results[category_np_lz4]["Decompression"] = parsed_results[f"Np_IAA_{i}"]["Decompression"]
        parsed_results[category_np_lz4]["Scan"] = parsed_results["LZ4"]["Scan"]
        parsed_results[category_np_lz4]["Mask"] = parsed_results["LZ4"]["Mask"]
        parsed_results[category_np_lz4]["Select"] = parsed_results["LZ4"]["Select"]
        parsed_results[category_np_lz4]["Sum"] = parsed_results["LZ4"]["Sum"]

stacked_x_labels = ["LZ4\n(CPU)\nPandas\n(CPU)", "Snappy\n(CPU)\nPandas\n(CPU)", "ZLIB\n(CPU)\nPandas\n(CPU)", "ZSTD\n(CPU)\nPandas\n(CPU)",
    "DEFLATE\n(CPU)\nQPL\n(CPU)", "DEFLATE\n(IAA_1)\nQPL\n(IAA_1)", "DEFLATE\n(IAA_2)\nQPL\n(IAA_2)", "DEFLATE\n(IAA_4)\nQPL\n(IAA_4)", "DEFLATE\n(IAA_8)\nQPL\n(IAA_8)",
    "DEFLATE\n(IAA_1)\nPandas\n(CPU)", "DEFLATE\n(IAA_2)\nPandas\n(CPU)", "DEFLATE\n(IAA_4)\nPandas\n(CPU)", "DEFLATE\n(IAA_8)\nPandas\n(CPU)",
    "Pipelined\n(IAA_1)", "Pipelined\n(IAA_2)", "Pipelined\n(IAA_4)", "Pipelined\n(IAA_8)"]

# Prepare data for plotting
x = np.arange(len(categories))
fig, ax = plt.subplots(figsize=(12, 6))

ax.bar(x, [parsed_results[cat]["Decompression"] for cat in categories], label="Decompression", color=colors["Decompression"])
ax.bar(x, [parsed_results[cat]["Scan"] for cat in categories], bottom=[parsed_results[cat]["Decompression"] for cat in categories], label="Scan", color=colors["Scan"])
ax.bar(x, [parsed_results[cat]["Mask"] for cat in categories], bottom=np.array([parsed_results[cat]["Decompression"] for cat in categories]) + np.array([parsed_results[cat]["Scan"] for cat in categories]), label="Mask", color=colors["Mask"])
ax.bar(x, [parsed_results[cat]["Select"] for cat in categories], bottom=np.array([parsed_results[cat]["Decompression"] for cat in categories]) + np.array([parsed_results[cat]["Scan"] for cat in categories]) + np.array([parsed_results[cat]["Mask"] for cat in categories]), label="Select", color=colors["Select"])
ax.bar(x, [parsed_results[cat]["Sum"] for cat in categories], bottom=np.array([parsed_results[cat]["Decompression"] for cat in categories]) + np.array([parsed_results[cat]["Scan"] for cat in categories]) + np.array([parsed_results[cat]["Mask"] for cat in categories]) + np.array([parsed_results[cat]["Select"] for cat in categories]), label="Sum", color=colors["Sum"])

ax.set_xticks(x)
ax.set_xticklabels(stacked_x_labels, ha="center", fontsize=7)
ax.set_ylabel("Elapsed Time (sec)")
ax.set_title("End_End_Application (Pandas)")
ax.legend()
plt.savefig("fig4.png", bbox_inches="tight")
plt.close()

print("Graph successfully created as 'fig4.png'!")
