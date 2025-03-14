import re
import matplotlib.pyplot as plt

# Define the input text file
file_path = "chaining_results.txt"

# Initialize variables to store speedup values
speedup_scan = None
speedup_extract = None

# Define regex pattern to extract speedup values
speedup_pattern = re.compile(r"\[IAA Pipeline Functionality \(Decompression \+ (\w+)\)\]\nSpeed up:\s*([\d\.]+)")

# Read the file and extract speedup values
with open(file_path, "r") as file:
    content = file.read()
    matches = speedup_pattern.findall(content)

    for match in matches:
        operation, speedup = match
        if operation == "Scan":
            speedup_scan = float(speedup)
        elif operation == "Extract":
            speedup_extract = float(speedup)

# Ensure both values were found
if speedup_scan is None or speedup_extract is None:
    raise ValueError("Could not find speedup values for both Decompression + Scan and Decompression + Extract.")

# Data for the plot
x_labels = ["Decompression + Scan", "Decompression + Extract"]
y_values = [speedup_scan, speedup_extract]
colors = ["black", "darkblue"]

# Create the bar plot
plt.figure(figsize=(6, 5))
plt.bar(x_labels, y_values, color=colors)
plt.ylabel("Speedup")
plt.title("IAA Pipeline Functionality Speedup")
plt.ylim(0, max(y_values) * 1.2)  # Adjust y-axis for better visibility
plt.grid(axis="y", linestyle="--", alpha=0.7)

# Save the plot as a PNG file
plt.savefig("fig3.png")
plt.close()

print("Graph successfully created and saved as 'fig3.png'!")
