#!/usr/bin/env bash
# Superseded by tests/run_tests.sh. The old Python tester compared only columns
# 1-3 of lines that had at least 3 fields, which silently skipped every
# undefined ('U') symbol. Kept as a thin wrapper so old habits still work.
exec bash "$(dirname "$0")/tests/run_tests.sh" "$@"
