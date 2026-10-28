#!/usr/bin/env python3
"""
Corrupted-ELF fuzzer for ft_nm.

usage: fuzz.py FT_NM_BINARY SEED_ELF [SEED_ELF ...]
env:   ITER  mutations per seed (default 40)

Every seed is mutated (header bytes, random bytes, truncation, hostile
offsets/counts) and fed to ft_nm with no flag, -a and -r. A run FAILS if the
process dies from a signal, hangs (>10 s), or -- when the binary is built with
sanitizers -- prints an AddressSanitizer / LeakSanitizer / UBSan report.
Deterministic (fixed seed), so a failure reproduces.

Exit status: 0 = no crash / hang, 1 = at least one.
"""
import os
import random
import subprocess
import sys
import tempfile

random.seed(1337)

if len(sys.argv) < 3:
    sys.exit(__doc__)

ft_nm = sys.argv[1]
seeds = sys.argv[2:]
iters = int(os.environ.get("ITER", "40"))
env = dict(os.environ,
           ASAN_OPTIONS="detect_leaks=1:exitcode=99",
           UBSAN_OPTIONS="print_stacktrace=1")

runs = crashes = hangs = 0
examples = []
tmp = tempfile.NamedTemporaryFile(prefix="ftnm_fuzz_", suffix=".bin", delete=False)
tmp.close()

HEADER_FIELDS = (0x10, 0x18, 0x20, 0x28, 0x38, 0x3A, 0x3C)  # type, entry, phoff, shoff, phnum, shentsize, shnum


def mutate(data: bytearray, mode: int) -> bytearray:
    d = bytearray(data)
    if mode == 0:        # corrupt the ELF header
        for _ in range(random.randint(1, 8)):
            d[random.randrange(min(64, len(d)))] = random.randrange(256)
    elif mode == 1:      # corrupt anywhere
        for _ in range(random.randint(1, 30)):
            d[random.randrange(len(d))] = random.randrange(256)
    elif mode == 2:      # truncate
        d = d[:random.randrange(1, len(d))]
    else:                # hostile offsets / counts
        for off in HEADER_FIELDS:
            if off < len(d):
                d[off] = random.choice([0xFF, 0x00, 0x7F, 0x80])
            if off + 7 < len(d):
                d[off + 7] = random.choice([0xFF, 0x00, 0x7F, 0x80])
    return d


try:
    for seed in seeds:
        with open(seed, "rb") as f:
            data = bytearray(f.read())[:300000]
        if len(data) < 64:
            continue
        for i in range(iters):
            with open(tmp.name, "wb") as f:
                f.write(mutate(data, i % 4))
            for flags in ([], ["-a"], ["-r"]):
                runs += 1
                try:
                    r = subprocess.run([ft_nm] + flags + [tmp.name],
                                       capture_output=True, timeout=10, env=env)
                except subprocess.TimeoutExpired:
                    hangs += 1
                    examples.append(("HANG", os.path.basename(seed), i, flags))
                    continue
                err = r.stderr.decode("latin1")
                if (r.returncode < 0 or r.returncode >= 99
                        or "Sanitizer" in err or "runtime error" in err):
                    crashes += 1
                    if len(examples) < 8:
                        why = [l.strip() for l in err.splitlines()
                               if "ERROR" in l or "runtime error" in l][:2]
                        examples.append(("CRASH rc=%d" % r.returncode,
                                         os.path.basename(seed), i, flags, why))
finally:
    os.unlink(tmp.name)

print("fuzz: runs=%d crashes=%d hangs=%d" % (runs, crashes, hangs))
for e in examples:
    print("   ", e)
sys.exit(1 if (crashes or hangs) else 0)
