#!/bin/bash

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 file1 [file2 ...]"
    exit 1
fi

for file in "$@"; do
    echo "Comparing: $file"
    diff <(./ft_nm -a "$file" | awk '{print $2, $3}') <(nm -a "$file" | awk '{print $2, $3}')
    echo
done
