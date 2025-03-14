import re
import argparse

# Set up argument parser
parser = argparse.ArgumentParser(description="Parse timing information from a .txt file")
parser.add_argument("input_file", type=str, help="Path to the input .txt file")
parser.add_argument("output_file", type=str, help="Path to the output .txt file")
args = parser.parse_args()

# Initialize a list to store timings
timings = 0
wait_timings = 0
init_timings = 0

# Open the input file and parse for timing information
with open(args.input_file, 'r') as file:
    for line in file:
        match = re.search(r"Elapsed Time:\s([\d.]+)\s", line)
        if match:
            timings += (float(match.group(1)))
        match = re.search(r"Wait Time:\s([\d.]+)\s", line)
        if match:
            wait_timings += (float(match.group(1)))
        match = re.search(r"Init Time:\s([\d.]+)\s", line)
        if match:
            init_timings += (float(match.group(1)))

# Write timings to the output file
with open(args.output_file, 'w') as output_file:
    output_file.write(f"Init Time: {init_timings} ms\n")
    output_file.write(f"Wait Time: {wait_timings} ms\n")
    output_file.write(f"Time: {timings} ms\n")