#!/usr/bin/env bash
# ft_nm test suite -- one command, exit status 0 only if everything passes.
#
#   bash tests/run_tests.sh              full run (~3 min under Docker Desktop)
#   bash tests/run_tests.sh --quick      smaller corpus / fewer fuzz rounds
#   bash tests/run_tests.sh --no-fuzz    skip the sanitizer build + fuzzing
#
# Needs: bash, gcc, make, GNU nm (the reference), python3 (fuzzing only).
# Easiest: `make docker-test` (or `docker compose run --rm nm`), which runs
# this same script inside a pinned image that has all of the above.
#
# What it checks
#   1. build      default flags (-Wall -Wextra -Werror) are warning-free; a
#                 FRESH CLONE (git-tracked files only) builds a working ft_nm;
#                 touching a source file relinks.
#   2. exact diff stdout+stderr+exit status of ft_nm byte-for-byte against the
#                 system nm, for every flag the subject asks for (-a -g -u -r
#                 -p and combinations), over compiled fixtures (all symbol
#                 classes, 64- and 32-bit), the committed bin/ fixtures, and a
#                 sample of real system binaries and shared libraries.
#   3. edge cases missing file, empty file, directory, text file, truncated
#                 ELF, FIFO, /dev/null, several files, bad option.
#   4. fuzz       corrupted ELFs against an ASan+UBSan build: no crash, no
#                 hang, no sanitizer report.

set -u
cd "$(dirname "$0")/.." || exit 1
ROOT=$PWD
QUICK=0; FUZZ=1
for a in "$@"; do
	case $a in
		--quick) QUICK=1 ;;
		--no-fuzz) FUZZ=0 ;;
		-h|--help) sed -n '2,25p' "$0"; exit 0 ;;
		*) echo "unknown option: $a" >&2; exit 2 ;;
	esac
done

if [ -t 1 ]; then G=$'\033[32m'; R=$'\033[31m'; Y=$'\033[33m'; B=$'\033[1m'; N=$'\033[0m'; else G=; R=; Y=; B=; N=; fi
PASS=0; FAIL=0; WARN=0
pass() { PASS=$((PASS+1)); printf '  %s[ ok ]%s %s\n' "$G" "$N" "$1"; }
fail() { FAIL=$((FAIL+1)); printf '  %s[FAIL]%s %s\n' "$R" "$N" "$1"; }
warn() { WARN=$((WARN+1)); printf '  %s[warn]%s %s\n' "$Y" "$N" "$1"; }
hdr()  { printf '\n%s== %s ==%s\n' "$B" "$1" "$N"; }

WORK=$(mktemp -d "${TMPDIR:-/tmp}/ftnm_tests.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
FT=$ROOT/ft_nm

# ------------------------------------------------------------- preflight --
hdr "environment"
for t in gcc make nm; do
	command -v $t >/dev/null || { echo "missing tool: $t (try: make docker-test)"; exit 2; }
done
echo "  $(nm --version | head -1)   $(gcc --version | head -1)   $(uname -sm)"
HAVE_PY=0; command -v python3 >/dev/null && HAVE_PY=1
[ $HAVE_PY = 1 ] || warn "python3 not found: fuzzing will be skipped"

# ----------------------------------------------------------------- build --
hdr "1. build"
if make re >"$WORK/build.log" 2>&1 && [ -x "$FT" ]; then pass "make re builds ./ft_nm"; else
	fail "make re did not produce ./ft_nm"; tail -20 "$WORK/build.log"; echo "RESULT: FAIL"; exit 1; fi
if grep -q -E '(warning|error):' "$WORK/build.log"; then fail "build is not warning-free"; else pass "build is warning-free (-Wall -Wextra -Werror)"; fi

if git -C "$ROOT" rev-parse --git-dir >/dev/null 2>&1 && [ -n "$(git -C "$ROOT" ls-files | head -1)" ]; then
	FRESH=$WORK/fresh; mkdir -p "$FRESH"
	git -C "$ROOT" ls-files -z --cached | (cd "$ROOT" && tar --null --ignore-failed-read -T - -cf - 2>/dev/null) | tar -xf - -C "$FRESH"
	if (cd "$FRESH" && make >/dev/null 2>&1) && [ -x "$FRESH/ft_nm" ]; then
		pass "fresh clone (git-tracked files only): make produces ft_nm"
		sleep 1; touch "$FRESH/src/main.c"
		before=$(stat -c %Y "$FRESH/ft_nm" 2>/dev/null || stat -f %m "$FRESH/ft_nm")
		(cd "$FRESH" && make >/dev/null 2>&1)
		after=$(stat -c %Y "$FRESH/ft_nm" 2>/dev/null || stat -f %m "$FRESH/ft_nm")
		[ "$after" -gt "$before" ] && pass "editing a source file relinks ft_nm" || fail "editing a source file did NOT relink (stale binary)"
	else
		fail "fresh clone: make did not produce ft_nm (committed sentinel/object files?)"
	fi
else
	warn "not a git checkout: fresh-clone build test skipped"
fi

# ------------------------------------------------------------- fixtures --
FX=$WORK/fx; mkdir -p "$FX"
cp "$ROOT"/tests/fixtures/rich.c "$FX/"
(
	cd "$FX" || exit
	gcc -fcommon -c rich.c -o rich.o 2>/dev/null
	gcc -fcommon -g -c rich.c -o rich_g.o 2>/dev/null
	gcc -fcommon rich.c -o rich.out 2>/dev/null
	gcc -fcommon -shared -fPIC rich.c -o rich.so 2>/dev/null
	[ -f rich.out ] && strip -o rich_stripped.out rich.out 2>/dev/null
	if gcc -m32 -fcommon -c rich.c -o rich32.o 2>/dev/null; then
		gcc -m32 -fcommon -shared -fPIC rich.c -o rich32.so 2>/dev/null
		gcc -m32 -fcommon rich.c -o rich32.out 2>/dev/null
	fi
)
cp "$ROOT"/bin/* "$FX/" 2>/dev/null
FIXTURES=$(cd "$FX" && for f in *; do head -c4 "$f" 2>/dev/null | grep -q ELF && echo "$f"; done)
NFX=$(echo "$FIXTURES" | wc -l)
ls "$FX"/rich32.o >/dev/null 2>&1 && HAVE32=yes || HAVE32=no

# system corpus: every k-th ELF under /usr/bin and /usr/lib (deterministic)
NSYS=$([ $QUICK = 1 ] && echo 40 || echo 160)
find /usr/bin /usr/lib /usr/lib64 /lib -maxdepth 4 -type f \( -name '*.so*' -o -perm -u+x \) 2>/dev/null | sort | \
	while read -r f; do [ ! -L "$f" ] && head -c4 "$f" 2>/dev/null | grep -q ELF && echo "$f"; done | \
	awk 'NR%5==1' | head -n "$NSYS" > "$WORK/sys.txt"
NSYSF=$(wc -l < "$WORK/sys.txt")

# one comparison: ft_nm vs nm over the same args, in $FX
# returns 0 identical, 1 different (diff saved in $WORK/last.diff)
compare() {
	( cd "$FX" && timeout 20 nm "$@" >"$WORK/e.txt" 2>&1; echo "rc=$?" >>"$WORK/e.txt" )
	( cd "$FX" && timeout 20 "$FT" "$@" >"$WORK/g.txt" 2>&1; echo "rc=$?" >>"$WORK/g.txt" )
	sed -i 's#^ft_nm:#nm:#' "$WORK/g.txt"
	# binutils' LTO plugin prints "bfd plugin: <file>: file too short" on tiny/broken ELFs
	sed -i '/^bfd plugin:/d' "$WORK/e.txt"
	cmp -s "$WORK/e.txt" "$WORK/g.txt" && return 0
	diff "$WORK/e.txt" "$WORK/g.txt" >"$WORK/last.diff"; return 1
}

# --------------------------------------------------------------- exact diff --
hdr "2. byte-exact output vs the system nm"
echo "  corpus: $NFX fixtures (32-bit built: $HAVE32) + $NSYSF system binaries/libraries"
SHOWN=0
for flags in "" "-a" "-g" "-u" "-r" "-p" "-a -r" "-g -r" "-u -r" "-a -p" "-ar" "-gu" "-rp" "-agr"; do
	bad=0; tot=0
	for f in $FIXTURES $(cat "$WORK/sys.txt"); do
		tot=$((tot+1))
		arg=$f
		# shellcheck disable=SC2086
		if ! compare $flags "$arg"; then
			bad=$((bad+1))
			if [ $SHOWN -lt 4 ]; then SHOWN=$((SHOWN+1)); printf '        --- nm %s %s\n' "$flags" "$arg"; head -8 "$WORK/last.diff" | sed 's/^/        /'; fi
		fi
	done
	if [ $bad = 0 ]; then pass "flags [${flags:-none}]: $tot/$tot identical"; else fail "flags [${flags:-none}]: $bad of $tot differ"; fi
done

# ---------------------------------------------------------------- edge cases --
hdr "3. edge cases (stdout + stderr + exit status)"
: > "$FX/empty"; mkdir -p "$FX/adir"; echo "not an elf" > "$FX/text.txt"
head -c 40 /bin/ls > "$FX/truncated"; mkfifo "$FX/fifo" 2>/dev/null
edge() { # label, expect_strict(1/0), args...
	local label=$1 strict=$2; shift 2
	if compare "$@"; then pass "$label"
	elif [ "$strict" = 0 ]; then warn "$label  (differs; GNU nm behaviour varies by binutils version)"; head -4 "$WORK/last.diff" | sed 's/^/        /'
	else fail "$label"; head -6 "$WORK/last.diff" | sed 's/^/        /'; fi
}
edge "nonexistent file"                1 /nonexistent_file
edge "empty file"                      0 empty
edge "directory"                       1 adir
edge "text file"                       1 text.txt
edge "truncated ELF"                   1 truncated
edge "FIFO (must not block)"           1 fifo
edge "/dev/null"                       1 /dev/null
edge "no arguments (a.out missing)"    1
edge "two files (headers + blank line)" 1 rich.o rich.so
edge "missing file between two files"  1 rich.o /nonexistent_file rich.so
edge "stripped binary (no symbols)"    1 rich_stripped.out
edge "-a with several files"           1 -a rich.o rich.out
edge "flag after file name"            1 rich.o -a
# nm's full usage text changes between binutils releases: compare the first
# line ("nm: invalid option -- 'z'") and the exit status only.
if ( cd "$FX" && nm -z rich.o 2>&1 | head -1; nm -z rich.o >/dev/null 2>&1; echo "rc=$?" ) >"$WORK/e.txt" 2>&1 	&& ( cd "$FX" && "$FT" -z rich.o 2>&1 | head -1; "$FT" -z rich.o >/dev/null 2>&1; echo "rc=$?" ) >"$WORK/g.txt" 2>&1 	&& sed -i 's#^ft_nm:#nm:#' "$WORK/g.txt" && cmp -s "$WORK/e.txt" "$WORK/g.txt"; then pass "invalid option (first line + exit status)"
else fail "invalid option (first line + exit status)"; diff "$WORK/e.txt" "$WORK/g.txt" | head -4 | sed 's/^/        /'; fi

# ------------------------------------------------------------------- fuzz --
hdr "4. corrupted-ELF fuzzing (ASan + UBSan build)"
if [ $FUZZ = 0 ]; then warn "skipped (--no-fuzz)"
elif [ $HAVE_PY = 0 ]; then warn "skipped (no python3)"
else
	SAN=$WORK/san; mkdir -p "$SAN"
	tar --exclude=./objects --exclude=./ft_nm --exclude=./.git --exclude=./intra_repo -C "$ROOT" -cf - . | tar -xf - -C "$SAN"
	if (cd "$SAN" && make fclean >/dev/null 2>&1; make -e CFLAGS='-fsanitize=address,undefined -fno-sanitize=alignment -fno-sanitize-recover=undefined -g -Wall -Wextra' >"$WORK/san.log" 2>&1) && [ -x "$SAN/ft_nm" ]; then
		SEEDS="$FX/rich.o $FX/rich.so $FX/rich.out $FX/rich_stripped.out $FX/header"
		[ "$HAVE32" = yes ] && SEEDS="$SEEDS $FX/rich32.o $FX/rich32.so"
		SEEDS="$SEEDS $(head -n 2 "$WORK/sys.txt" | tr '\n' ' ')"
		ITER=$([ $QUICK = 1 ] && echo 10 || echo 40) python3 "$ROOT/tests/fuzz.py" "$SAN/ft_nm" $SEEDS >"$WORK/fuzz.log" 2>&1
		rc=$?; cat "$WORK/fuzz.log" | sed 's/^/  /'
		[ $rc = 0 ] && pass "no crash, hang or sanitizer report" || fail "fuzzing found crashes/hangs (see above)"
	else
		fail "sanitizer build failed"; tail -15 "$WORK/san.log"
	fi
	echo "  note: -fno-sanitize=alignment -- hostile offsets can make ELF structs misaligned; harmless on x86, see README"
fi

# --------------------------------------------------------------- summary --
hdr "summary"
printf '  passed: %s%d%s   failed: %s%d%s   warnings: %d\n' "$G" $PASS "$N" "$R" $FAIL "$N" $WARN
if [ $FAIL = 0 ]; then printf '  %sRESULT: PASS%s\n' "$G" "$N"; exit 0; fi
printf '  %sRESULT: FAIL%s\n' "$R" "$N"; exit 1
