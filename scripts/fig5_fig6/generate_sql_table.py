import os
import re
import argparse

def split_sql_file(input_file, output_dir, compression_type, data_path):
    """Splits a single SQL file into separate table-specific SQL files and replaces compression type and data file paths."""

    # Ensure output directory exists
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Read the input SQL file
    with open(input_file, "r", encoding="utf-8") as file:
        sql_content = file.read()

    # Replace compression type
    sql_content = re.sub(r"compression\s*=>\s*'lz4'", f"compression => '{compression_type}'", sql_content, flags=re.IGNORECASE)

    # Regular expression to match CREATE TABLE statements
    table_splits = re.split(r"(CREATE TABLE IF NOT EXISTS\s+\w+\s*\()", sql_content, flags=re.IGNORECASE)

    # Iterate through the extracted table definitions
    for i in range(1, len(table_splits), 2):
        table_header = table_splits[i].strip()
        table_body = table_splits[i + 1].strip()

        # Extract the table name
        table_name_match = re.search(r"CREATE TABLE IF NOT EXISTS\s+(\w+)", table_header, re.IGNORECASE)
        if table_name_match:
            table_name = table_name_match.group(1)
            table_sql = f"{table_header}{table_body}"

            # Modify the COPY statement with the provided data path
            table_sql = re.sub(
                rf"COPY {table_name} FROM\s+'[^']+\.tbl'",
                f"COPY {table_name} FROM '{data_path}/{table_name}.tbl'",
                table_sql,
                flags=re.IGNORECASE
            )

            # Define output file path
            output_file = os.path.join(output_dir, f"{table_name}.sql")

            # Save to individual SQL file
            with open(output_file, "w", encoding="utf-8") as out_file:
                out_file.write(table_sql.strip() + "\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Split SQL file into separate table scripts with configurable compression type and data file path.")
    parser.add_argument("input_file", help="Path to the input SQL file.")
    parser.add_argument("output_dir", help="Directory to store split SQL files.")
    parser.add_argument("compression_type", help="Compression type to replace 'lz4'.")
    parser.add_argument("data_path", help="Path to the directory containing the .tbl files.")

    args = parser.parse_args()
    split_sql_file(args.input_file, args.output_dir, args.compression_type, args.data_path)
