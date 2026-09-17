#!/usr/bin/env python3
"""
bash_tester.py: Compare system 'nm' vs custom './ft_nm' outputs with colored output and summary.
Usage: python3 bash_tester.py [--ft-nm PATH] [--bin-dir PATH]
"""
import argparse
import subprocess
import sys
import os
from difflib import unified_diff

# ANSI color codes
green = '\033[0;32m'
red = '\033[0;31m'
nc = '\033[0m'

# List of flags to test
FLAGS = ['', '-a', '-g', '-u', '-r', '-p']


def run_nm(cmd, args):
    """
    Run the given command (`nm` or ft_nm) with provided args list.
    Returns a list of strings containing columns 2 and 3 of each line.
    """
    try:
        res = subprocess.run([cmd] + args,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.DEVNULL,
                             check=False,
                             text=True)
        # take columns 2 and 3 (indexes 1 and 2) of each line
        lines = [ ' '.join(parts[1:3]) for parts in (line.split() for line in res.stdout.splitlines()) if len(parts) >= 3 ]
        return lines
    except Exception:
        return []

def run_test(ft_nm, file, flag, counters, fail_list):
    label = f"file='{file}' flags='{flag}'"
    counters['total'] += 1
    print(f"=== Testing: {label} ===")

    args = [flag, file] if flag else [file]

    # Get raw outputs from nm and ft_nm
    try:
        sys_res = subprocess.run(['nm'] + args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False, text=True)
        ft_res = subprocess.run([ft_nm] + args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False, text=True)
    except Exception:
        counters['fail'] += 1
        fail_list.append(label)
        print(f"[{red}ERROR{nc}] {label} - exception during subprocess")
        return

    sys_lines = [line for line in sys_res.stdout.splitlines() if 'plugin:' not in line and len(line.strip().split()) >= 3]
    ft_lines  = [line for line in ft_res.stdout.splitlines() if 'plugin:' not in line and len(line.strip().split()) >= 3]

    # Extract only columns 2 and 3
    sys_out = [' '.join(line.strip().split()[1:3]) for line in sys_lines]
    ft_out  = [' '.join(line.strip().split()[1:3]) for line in ft_lines]

    if sys_out == ft_out:
        counters['pass'] += 1
        print(f"[{green}OK{nc}] {label}\n")
    else:
        # Check if the *only* difference was a plugin error
        if all("plugin:" in line for line in sys_res.stdout.splitlines() + ft_res.stdout.splitlines()):
            counters['pass'] += 1
            print(f"[{green}OK{nc}] {label} (plugin warning only)\n")
        else:
            counters['fail'] += 1
            fail_list.append(label)
            print(f"[{red}FAIL{nc}] {label}")
            diff = unified_diff(sys_out, ft_out,
                                fromfile='nm output',
                                tofile='ft_nm output',
                                lineterm='')
            print("--- unified diff ---")
            for line in diff:
                print(line)
            print()


def main():
    parser = argparse.ArgumentParser(description='Compare nm vs ft_nm outputs')
    parser.add_argument('--ft-nm', default='./ft_nm',
                        help='Path to the ft_nm executable')
    parser.add_argument('--bin-dir', default='./bin',
                        help='Directory containing binaries')
    args = parser.parse_args()

    if not os.path.isfile(args.ft_nm) or not os.access(args.ft_nm, os.X_OK):
        print(f"Error: '{args.ft_nm}' not found or not executable.", file=sys.stderr)
        sys.exit(1)

    counters = {'total': 0, 'pass': 0, 'fail': 0}
    fail_list = []

    # Mandatory: no flags
    for fname in os.listdir(args.bin_dir):
        path = os.path.join(args.bin_dir, fname)
        if os.path.isfile(path) and os.access(path, os.X_OK):
            run_test(args.ft_nm, path, '', counters, fail_list)

    # Bonus: flags
    print("\n===== Bonus: flag combinations =====")
    for fname in os.listdir(args.bin_dir):
        path = os.path.join(args.bin_dir, fname)
        if os.path.isfile(path) and os.access(path, os.X_OK):
            for flag in FLAGS:
                if not flag:
                    continue
                run_test(args.ft_nm, path, flag, counters, fail_list)

    # Summary
    print("="*42)
    print(f"Total tests: {counters['total']}")
    print(f"Passed: {green}{counters['pass']}{nc}")
    print(f"Failed: {red}{counters['fail']}{nc}")
    if counters['fail'] > 0:
        print("\nFailed cases:")
        for case in fail_list:
            print(f"  - {red}{case}{nc}")

if __name__ == '__main__':
    main()

