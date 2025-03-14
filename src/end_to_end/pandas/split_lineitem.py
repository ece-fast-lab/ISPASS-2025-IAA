import pandas as pd
import numpy as np
import sys
import os

def main():
    # Change directory
    input_lineitem_dir = sys.argv[1]
    output_lineitem_dir = sys.argv[2]

    # Ensure the output directory exists
    if not os.path.exists(output_lineitem_dir):
        os.makedirs(output_lineitem_dir)
        print(f"Created directory: {output_lineitem_dir}")

    # Load the .tbl file into a pandas DataFrame
    df = pd.read_csv(input_lineitem_dir, sep="|", header=None)

    # Define column names
    column_names = ['l_orderkey', 'l_partkey', 'l_suppkey', 'l_linenumber', 'l_quantity', 'l_extendedprice', 
                    'l_discount', 'l_tax', 'l_returnflag', 'l_linestatus', 'l_shipdate', 'l_commitdate', 
                    'l_receiptdate', 'l_shipinstruct', 'l_shipmode', 'l_comment']

    # Loop through each column in the DataFrame
    for i, col_name in enumerate(df.columns):
        # Convert date columns to timestamps
        if 10 <= col_name <= 12:
            df[col_name] = pd.to_datetime(df[col_name], format='%Y-%m-%d')
            df[col_name] = df[col_name].astype('int64') // 10**9  # Convert to Unix timestamp

        # Convert data to bytes
        if df[col_name].dtype == 'int64':
            col_bytes = df[col_name].astype(np.int32).values.tobytes()  # Convert int64 to int32
        elif df[col_name].dtype == 'float64':
            col_bytes = df[col_name].astype(np.float32).values.tobytes()  # Convert float64 to float32
        else:
            col_bytes = b''.join([str(x).encode('utf-8') for x in df[col_name].values])  # Encode as UTF-8

        # Write bytes to a file in the output directory
        output_path = os.path.join(output_lineitem_dir, column_names[i] + ".bin")
        with open(output_path, "wb") as f:
            f.write(col_bytes)

        print(f"Column {column_names[i]} written to {output_path}")

if __name__ == "__main__":
    main()
