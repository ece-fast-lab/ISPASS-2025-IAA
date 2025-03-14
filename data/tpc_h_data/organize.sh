#!/bin/bash

# Loop through all .tbl files in the current directory
for i in *.tbl; do
    # Check if there are any .tbl files
    if [[ ! -e "$i" ]]; then
        echo "No .tbl files found in the directory."
        exit 1
    fi
    
    # Skip processing if the file is not 'lineitem.tbl'
    # if [[ "$i" != "lineitem.tbl" ]]; then
    #     continue
    # fi

    # Use sed to remove the trailing '|' and save directly back into the original file
    sed -i 's/|$//' "$i"
    
    # Print the name of the processed file
    echo "Processed: $i"
done

# for i in *.dat; do
#     # Check if there are any .dat files
#     if [[ ! -e "$i" ]]; then
#         echo "No .dat files found in the directory."
#         exit 1
#     fi
    
#     # Skip processing if the file is not 'lineitem.dat'
#     # if [[ "$i" != "lineitem.dat" ]]; then
#     #     continue
#     # fi

#     # Use sed to remove the trailing '|' and save directly back into the original file
#     sed -i 's/|$//' "$i"
    
#     # Print the name of the processed file
#     echo "Processed: $i"
# done