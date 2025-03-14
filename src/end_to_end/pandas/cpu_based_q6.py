import pandas as pd
from datetime import datetime
import time
import os
import lz4.frame
import zlib
import snappy
from io import StringIO
import numpy as np
import concurrent.futures
import sys
import zstd

# from concurrent.futures import ThreadPoolExecutor, as_completed


column_names = [
    'l_orderkey', 'l_partkey', 'l_suppkey', 'l_linenumber', 
    'l_quantity', 'l_extendedprice', 'l_discount', 'l_tax', 
    'l_returnflag', 'l_linestatus', 'l_shipdate', 'l_commitdate', 
    'l_receiptdate', 'l_shipinstruct', 'l_shipmode', 'l_comment'
]

max_char_lengths = {
    'l_shipinstruct': 25,  # CHAR(25)
    'l_shipmode': 10,      # CHAR(10)
    'l_comment': 44        # VARCHAR(44)
}

int32_columns = ['l_orderkey', 'l_partkey', 'l_suppkey', 'l_linenumber', 'l_quantity']
float32_columns = ['l_extendedprice', 'l_discount', 'l_tax']
s1_columns = ['l_returnflag', 'l_linestatus']
datetime_columns = ['l_shipdate', 'l_commitdate', 'l_receiptdate']

# int32_columns = ['l_quantity']
# float32_columns = ['l_extendedprice', 'l_discount']
# s1_columns = []
# datetime_columns = ['l_shipdate']

# Compress the file
def compress_lz4(input_file, output_file):
    with open(input_file, 'rb') as f_in:
        data = f_in.read()
    
    compressed_data = lz4.frame.compress(data)
    
    with open(output_file, 'wb') as f_out:
        f_out.write(compressed_data)

# Decompress the file and measure time taken
def decompress_lz4(compressed_data):

    start_time = time.time()

    decompressed_data = lz4.frame.decompress(compressed_data)
    
    end_time = time.time()
    time_taken = end_time - start_time
    
    return time_taken

# Compress the file
def compress_zstd(input_file, output_file):
    with open(input_file, 'rb') as f_in:
        data = f_in.read()


    # Compress the data
    compressed_data = zstd.compress(data)

    with open(output_file, 'wb') as f_out:
        f_out.write(compressed_data)

# Decompress the file and measure time taken
def decompress_zstd(compressed_data):

    # Measure the decompression time
    start_time = time.time()

    # Decompress the data
    decompressed_data = zstd.decompress(compressed_data)

    end_time = time.time()
    time_taken = end_time - start_time

    return time_taken

# Function to compress using zlib
def compress_zlib(input_file, output_file):
    with open(input_file, 'rb') as f_in, open(output_file, 'wb') as f_out:
        data = f_in.read()
        compressed_data = zlib.compress(data)
        f_out.write(compressed_data)

# Function to decompress using zlib
def decompress_zlib(compressed_data):
    start_time = time.time()

    decompressed_data = zlib.decompress(compressed_data)
    
    end_time = time.time()
    time_taken = end_time - start_time
    
    return time_taken

# Function to compress using snappy
def compress_snappy(input_file, output_file):
    with open(input_file, 'rb') as f_in, open(output_file, 'wb') as f_out:
        data = f_in.read()
        compressed_data = snappy.compress(data)
        f_out.write(compressed_data)

# Function to decompress using snappy
def decompress_snappy(compressed_data):
    start_time = time.time()

    decompressed_data = snappy.decompress(compressed_data)
    
    end_time = time.time()
    time_taken = end_time - start_time    
    return time_taken


# Compress all files in the directory
def compress_lz4_in_directory(directory_path):
    output_dir = directory_path + "lz4_compressed_line_item/"
    os.makedirs(output_dir, exist_ok=True)
    for filename in os.listdir(directory_path):
        full_path = os.path.join(directory_path, filename)
        
        # Only process files, not subdirectories
        if os.path.isfile(full_path):
            output_path = os.path.join(output_dir, filename) + '.lz4'
            compress_lz4(full_path, output_path)

# Decompress all .lz4 files in the directory and measure time
def decompress_lz4_in_directory(directory_path, data, num_threads=4 ):
    print(directory_path)
    compressed_files = [
        os.path.join(directory_path, filename) for filename in os.listdir(directory_path)
        if filename.endswith('.lz4')
    ]
    len_dir_path = len(directory_path)
    len_method = len('.lz4') + len('.bin')
    compressed_data_list = []
    for compressed_file in compressed_files:
        file_name = compressed_file[len_dir_path: -len_method]
        if not (len(file_name) > 0 and file_name[0] == 'l'):
            continue
        if not (file_name in int32_columns or file_name in float32_columns or file_name in datetime_columns or file_name in s1_columns):
            continue
        with open(compressed_file, 'rb') as f_in:
            compressed_data = f_in.read()
            compressed_data_list.append(compressed_data)
        decompressed_data = lz4.frame.decompress(compressed_data)
        if file_name in int32_columns:
            dtype = np.int32  # Integer columns
        elif file_name in float32_columns:
            dtype = np.float32  # Float columns
        elif file_name in datetime_columns:
            dtype = np.int32  # Timestamps are stored as int64
        elif file_name in s1_columns:
            dtype = 'S1'
        else:
            continue
        
        col_data = np.frombuffer(decompressed_data, dtype=dtype)

        if file_name in datetime_columns:
            col_data = pd.to_datetime(col_data, unit='s')

        if dtype == 'S':
            col_data = col_data.astype(str)

        # Store the column data in the dictionary
        data[file_name] = col_data

    total_time = 0

    for compressed_data in compressed_data_list:
        total_time += decompress_lz4(compressed_data)

    print(f"Time took for single threaded LZ4 decompression: {total_time:.4f} sec")
    return total_time

def compress_zlib_in_directory(directory_path):
    output_dir = directory_path+ "zlib_compressed_line_item/"
    os.makedirs(output_dir, exist_ok=True)
    for filename in os.listdir(directory_path):
        full_path = os.path.join(directory_path, filename)
        
        # Only process files, not subdirectories
        if os.path.isfile(full_path):
            output_path = os.path.join(output_dir, filename) + '.zlib'
            compress_zlib(full_path, output_path)

def decompress_zlib_in_directory(directory_path, data, num_threads=4 ):

    compressed_files = [
        os.path.join(directory_path, filename) for filename in os.listdir(directory_path)
        if filename.endswith('.zlib')
    ]
    len_dir_path = len(directory_path)
    len_method = len('.zlib') + len('.bin')
    compressed_data_list = []
    for compressed_file in compressed_files:
        file_name = compressed_file[len_dir_path: -len_method]
        if not (len(file_name) > 0 and file_name[0] == 'l'):
            continue
        if not (file_name in int32_columns or file_name in float32_columns or file_name in datetime_columns or file_name in s1_columns):
            continue
        with open(compressed_file, 'rb') as f_in:
            compressed_data = f_in.read()
            compressed_data_list.append(compressed_data)
        decompressed_data = zlib.decompress(compressed_data)
        if file_name in int32_columns:
            dtype = np.int32  # Integer columns
        elif file_name in float32_columns:
            dtype = np.float32  # Float columns
        elif file_name in datetime_columns:
            dtype = np.int32  # Timestamps are stored as int64
        elif file_name in s1_columns:
            dtype = 'S1'
        else:
            continue
        
        col_data = np.frombuffer(decompressed_data, dtype=dtype)

        if file_name in datetime_columns:
            col_data = pd.to_datetime(col_data, unit='s')

        if dtype == 'S':
            col_data = col_data.astype(str)

        # Store the column data in the dictionary
        data[file_name] = col_data

    total_time = 0

    for compressed_data in compressed_data_list:
        total_time += decompress_zlib(compressed_data)

    print(f"Time took for single threaded ZLIB decompression: {total_time:.4f} sec")
    return total_time

def compress_snappy_in_directory(directory_path):
    output_dir = directory_path + "snappy_compressed_line_item/"
    os.makedirs(output_dir, exist_ok=True)
    for filename in os.listdir(directory_path):
        full_path = os.path.join(directory_path, filename)
        
        # Only process files, not subdirectories
        if os.path.isfile(full_path):
            output_path = os.path.join(output_dir, filename) + '.snappy'
            compress_snappy(full_path, output_path)

def decompress_snappy_in_directory(directory_path, data, num_threads=4 ):

    compressed_files = [
        os.path.join(directory_path, filename) for filename in os.listdir(directory_path)
        if filename.endswith('.snappy')
    ]
    len_dir_path = len(directory_path)
    len_method = len('.snappy') + len('.bin')
    compressed_data_list = []
    for compressed_file in compressed_files:
        file_name = compressed_file[len_dir_path: -len_method]
        if not (len(file_name) > 0 and file_name[0] == 'l'):
            continue
        if not (file_name in int32_columns or file_name in float32_columns or file_name in datetime_columns or file_name in s1_columns):
            continue
        with open(compressed_file, 'rb') as f_in:
            compressed_data = f_in.read()
            compressed_data_list.append(compressed_data)
        file_name = compressed_file[len_dir_path: -len_method]
        decompressed_data = snappy.decompress(compressed_data)
        if file_name in int32_columns:
            dtype = np.int32  # Integer columns
        elif file_name in float32_columns:
            dtype = np.float32  # Float columns
        elif file_name in datetime_columns:
            dtype = np.int32  # Timestamps are stored as int64
        elif file_name in s1_columns:
            dtype = 'S1'
        else:
            continue
        
        col_data = np.frombuffer(decompressed_data, dtype=dtype)

        if file_name in datetime_columns:
            col_data = pd.to_datetime(col_data, unit='s')

        if dtype == 'S':
            col_data = col_data.astype(str)

        # Store the column data in the dictionary
        data[file_name] = col_data

    total_time = 0

    for compressed_data in compressed_data_list:
        total_time += decompress_snappy(compressed_data)
    print(f"Time took for single threaded SNAPPY decompression: {total_time:.4f} sec")
    return total_time

def compress_zstd_in_directory(directory_path):
    output_dir = directory_path + "zstd_compressed_line_item/"
    os.makedirs(output_dir, exist_ok=True)
    for filename in os.listdir(directory_path):
        full_path = os.path.join(directory_path, filename)
        
        # Only process files, not subdirectories
        if os.path.isfile(full_path):
            output_path = os.path.join(output_dir, filename) + '.zstd'
            # print(f"Compressing {full_path} into {output_path}...")
            compress_zstd(full_path, output_path)

def decompress_zstd_in_directory(directory_path, data, num_threads=4 ):

    compressed_files = [
        os.path.join(directory_path, filename) for filename in os.listdir(directory_path)
        if filename.endswith('.zstd')
    ]
    len_dir_path = len(directory_path)
    len_method = len('.zstd') + len('.bin')
    compressed_data_list = []
    for compressed_file in compressed_files:
        file_name = compressed_file[len_dir_path: -len_method]
        if not (len(file_name) > 0 and file_name[0] == 'l'):
            continue
        if not (file_name in int32_columns or file_name in float32_columns or file_name in datetime_columns or file_name in s1_columns):
            continue
        with open(compressed_file, 'rb') as f_in:
            compressed_data = f_in.read()
            compressed_data_list.append(compressed_data)
        decompressed_data = zstd.decompress(compressed_data)
        if file_name in int32_columns:
            dtype = np.int32  # Integer columns
        elif file_name in float32_columns:
            dtype = np.float32  # Float columns
        elif file_name in datetime_columns:
            dtype = np.int32  # Timestamps are stored as int64
        elif file_name in s1_columns:
            dtype = 'S1'
        else:
            continue
        
        col_data = np.frombuffer(decompressed_data, dtype=dtype)

        if file_name in datetime_columns:
            col_data = pd.to_datetime(col_data, unit='s')

        if dtype == 'S':
            col_data = col_data.astype(str)

        # Store the column data in the dictionary
        data[file_name] = col_data

    total_time = 0

    for compressed_data in compressed_data_list:
        total_time += decompress_zstd(compressed_data)
    print(f"Time took for single threaded ZSTD decompression: {total_time:.4f} sec")
    return total_time

def main():
    # python3 query6.py lz4 1
    directory_path = sys.argv[1]
    decompress_method = sys.argv[2]
    data = {}
    # Compress all files in the directory
    if decompress_method == 'lz4':
        print("LZ4 decompression")
        compress_lz4_in_directory(directory_path)
        compressed_dir_path = directory_path + 'lz4_compressed_line_item/'
        decompression_time = decompress_lz4_in_directory(compressed_dir_path, data, 1)
    elif decompress_method == 'snappy':
        print("SNAPPY decompression")
        compress_snappy_in_directory(directory_path)
        compressed_dir_path = directory_path + 'snappy_compressed_line_item/'
        decompression_time = decompress_snappy_in_directory(compressed_dir_path, data, 1)
    elif decompress_method == 'zstd':
        print("ZSTD decompression")
        compress_zstd_in_directory(directory_path)
        compressed_dir_path = directory_path + 'zstd_compressed_line_item/'
        decompression_time = decompress_zstd_in_directory(compressed_dir_path, data, 1)
    else:
        print("ZLIB decompression")
        compress_zlib_in_directory(directory_path)
        compressed_dir_path = directory_path + 'zlib_compressed_line_item/'
        decompression_time = decompress_zlib_in_directory(compressed_dir_path, data, 1)

    df = pd.DataFrame(data)

    # Apply the filtering conditions:
    # 1. l_shipdate between '1994-01-01' and '1995-01-01'
    # 2. l_discount between 0.05 and 0.07
    # 3. l_quantity < 24

    # Define the filter date range: '1994-01-01' <= l_shipdate < '1995-01-01'
    start_date = pd.to_datetime('1994-01-01')
    end_date = pd.to_datetime('1995-01-01')

    q6_filter_start_time = time.perf_counter() 
    condition = (df['l_shipdate'] >= start_date) & (df['l_shipdate'] < end_date) & (df['l_discount'] >= 0.05) & (df['l_discount'] <= 0.07) & (df['l_quantity'] < 24)
    q6_filter_end_time = time.perf_counter() 

    # Calculate the revenue: sum(l_extendedprice * l_discount) 
    q6_select_start_time = time.perf_counter() 
    filtered_df = df[condition]
    l_extendedprice = filtered_df['l_extendedprice']
    l_discount = filtered_df['l_discount']
    q6_select_end_time = time.perf_counter() 
    q6_sum_start_time = time.perf_counter() 
    revenue = (l_extendedprice* l_discount).sum()
    q6_sum_end_time = time.perf_counter() 
    q6_filter_time_taken = q6_filter_end_time - q6_filter_start_time
    q6_select_time_taken = q6_select_end_time - q6_select_start_time
    q6_sum_time_taken = q6_sum_end_time - q6_sum_start_time
    print(f"Time taken for decompression: {decompression_time:.4f} seconds")
    print(f"Time taken for q6 scan: {q6_filter_time_taken:.6f} seconds")
    print(f"Time taken for q6 select: {q6_select_time_taken:.6f} seconds")
    print(f"Time taken for q6 sum: {q6_sum_time_taken:.6f} seconds")
    q6_total = decompression_time + q6_filter_time_taken + q6_select_time_taken + q6_sum_time_taken
    print(f"Time taken for q6 total: {q6_total:.6f} seconds")
    # Print the revenue
    # print(f"Revenue: {revenue:.4f}")

if __name__ == "__main__":
    main()
