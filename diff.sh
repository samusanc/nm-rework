#!/bin/bash
# Byte-exact diff of ft_nm against the system nm for the given files.
# (The old version sliced columns with awk, which hid real differences.)
# usage: ./diff.sh [nm options] file...   e.g.  ./diff.sh -a /bin/ls
[ "$#" -eq 0 ] && { echo "Usage: $0 [nm options] file1 [file2 ...]"; exit 1; }
diff <(nm "$@" 2>&1 | sed '/^bfd plugin:/d') <(./ft_nm "$@" 2>&1 | sed 's/^ft_nm:/nm:/') && echo "identical"
