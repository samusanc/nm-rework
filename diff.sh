#!/bin/bash

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 file1 [file2 ...]"
    exit 1
fi

# WHY THIS CHANGED: `awk '{print $2, $3}'` dropped $1, the address
# column, from the comparison -- the exact column that was silently
# wrong (decimal instead of hex) in src/utils/print_utils.c. Same root
# cause and same fix as tester.sh: compare all three columns.
for file in "$@"; do
    echo "Comparing: $file"
    diff <(./ft_nm -a "$file" | awk '{print $1, $2, $3}') <(nm -a "$file" | awk '{print $1, $2, $3}')
    echo
done
