# ft_nm — hardening notes

This repo passed defense on the intra version. This README is a post-defense
learning pass: every crash, hang, and correctness bug found by auditing the
original submission and testing it under ASan/UBSan against real binaries.
Each entry shows the original code, what actually goes wrong with it, the
fix, and the general lesson — the point of writing it this way is to make
each bug reproducible and explainable, not just "fixed".

If you want to reproduce any of this yourself:

```sh
CFLAGS='-fsanitize=address,undefined -g -Wall -Wextra' make -e re
./ft_nm /bin/ls          # stripped binary
./ft_nm /some/directory
: > /tmp/empty && ./ft_nm /tmp/empty
```

---

## 1. `mmap()`'s return value was never checked

**File:** `src/main.c`, `ft_nm()`

```c
int fd = open(file, O_RDONLY);
if (fd < 0)
    return error(file, "No such file", 1);

struct stat st;
fstat(fd, &st);
void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
close(fd);

unsigned char *e_ident = (unsigned char *)mapped;
if (is_valid_elf(e_ident, file))   // dereferences mapped, unchecked
```

`fstat()`'s return value is also discarded, and `mmap()` can return
`MAP_FAILED` (`(void *)-1`) instead of a real pointer — which happens for an
empty file (`mmap(..., 0, ...)` is invalid), a directory opened `O_RDONLY`,
or a handful of other conditions. `is_valid_elf()` then dereferences that
sentinel value as if it were mapped memory.

**Reproduced:** `./ft_nm /tmp/empty_file` and `./ft_nm /some/directory` both
SIGSEGV under ASan at `is_valid_elf`, `src/main.c:44`.

**Fix:** `stat()` the file *before* opening it (see #9 below for why stat
before open, not fstat after), reject anything that isn't `S_ISREG`, reject
a size smaller than the smallest possible ELF header, and check `mmap()`
against `MAP_FAILED` before touching the pointer.

**Lesson:** every syscall that can fail (`open`, `fstat`, `mmap`) needs its
return value checked *before* the result is used, full stop. This is the
single most common way a "recode X in C" project fails the "must never
segfault" requirement — not the ELF parsing itself, but the file I/O around
it.

---

## 2. No NULL check on `symtab_hdr` / `strtab_hdr` — crashes on every stripped binary

**File:** `src/x64/x64_utils.c` / `src/x86/x86_utlis.c`, `process_elf64/32()`

```c
Elf64_Shdr *symtab_hdr = NULL;
Elf64_Shdr *strtab_hdr = NULL;

for (int i = 0; i < ehdr->e_shnum; i++) {
    const char *name = shstrtab + shdr[i].sh_name;
    if (shdr[i].sh_type == SHT_SYMTAB)
        symtab_hdr = &shdr[i];
    else if (shdr[i].sh_type == SHT_STRTAB && !(ft_strcmp(name, ".strtab")))
        strtab_hdr = &shdr[i];
}

Elf64_Sym *symbols = (Elf64_Sym *)((char *)mapped + symtab_hdr->sh_offset);
//                                                   ^^^^^^^^^^^^^^^^^^^^^
//                                    NULL->sh_offset if there's no .symtab
```

A binary with no `.symtab` section — i.e. **every stripped binary** — leaves
`symtab_hdr` as `NULL`, and the very next line dereferences it. This is the
scenario the subject explicitly calls out: *"on recent Ubuntu versions
(20.04+), most system binaries are stripped of symbols."* The project's own
`bin/` test fixtures never caught this because they're all unstripped `.o`
files compiled specifically for testing — the bug only shows up against
real-world binaries.

**Reproduced:** `./ft_nm /bin/ls`, `./ft_nm /lib/x86_64-linux-gnu/libc.so.6`
— both SIGSEGV at `x64_utils.c:102`.

**Fix:** if `symtab_hdr` or `strtab_hdr` is `NULL` after the scan, return
early with no error — that's not a malformed file, it's the completely
normal "no symbols" case, and real `nm` prints `nm: file: no symbols` and
exits 0 for it.

**Lesson:** test against real, unmodified system binaries early, not just
your own compiled fixtures. `/bin/ls` and `/lib/.../libc.so.6` are on every
Linux box and will immediately surface this class of bug.

---

## 3. Integer overflow lets a wild pointer through the bounds check

**File:** `src/x64/x64_utils.c` / `src/x86/x86_utlis.c`

```c
if (ehdr->e_shoff + (ehdr->e_shnum * sizeof(Elf64_Shdr)) > file_size)
    return error(file, "file format not recognized", 0);

Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)mapped + ehdr->e_shoff);
```

`e_shoff` is an attacker/fuzzer-controlled 64-bit field read straight from
the file. If it's close to `UINT64_MAX`, the addition wraps around to a
small number — the check passes — while `mapped + e_shoff` computes a
pointer *before* the start of the mapping.

**Reproduced:** a hand-crafted ELF header with `e_shoff = UINT64_MAX - 5`
made the program read an `Elf64_Shdr` from 5 bytes before the mapped
buffer; UBSan caught it as a misaligned access. It happened not to
immediately fault only because that address was still mapped in that
particular run — a different heap layout turns this into an unconditional
SIGSEGV.

**Fix:** a subtraction-based bounds check instead of an addition-based one:

```c
static int range_in_bounds(size_t off, size_t size, size_t file_size)
{
    if (off > file_size)
        return 0;
    if (size > file_size - off)      // can't overflow: off <= file_size here
        return 0;
    return 1;
}
```

Used for every offset+size pair pulled from the file: the section table,
`.shstrtab`, `.symtab`, `.strtab`.

**Lesson:** when validating `offset + size <= limit` where `offset`/`size`
come from untrusted input, never write it as an addition — rewrite it as
two subtractions so nothing can wrap around. This is a generic pattern, not
ELF-specific; it applies to parsing any binary format.

---

## 4. Unbounded string lookups (`sh_name` / `st_name`)

**File:** `src/x64/x64_utils.c` / `src/x86/x86_utlis.c`

```c
const char *name = shstrtab + shdr[i].sh_name;       // section names
...
name = strtab + sym.st_name;                          // symbol names
```

Neither offset is checked against the actual size of the string table, and
neither call verifies there's a NUL terminator before the mapping ends. A
corrupted or fuzzed offset, or a string table that isn't NUL-terminated at
the very edge of the file, walks `ft_strcmp`/`ft_strdup` straight off the
end of the `mmap`'d region.

**Fix:** a small helper used everywhere a string is resolved from a table:

```c
const char *safe_str(const char *base, size_t offset, size_t table_size)
{
    if (!base || offset >= table_size)
        return NULL;
    size_t remaining = table_size - offset;
    if (!memchr(base + offset, '\0', remaining))
        return NULL;                 // no terminator before the table ends
    return base + offset;
}
```

**Lesson:** an offset into a table is not a string until you've proven two
things: it's in range, *and* it's terminated before the range ends. Bounds
checking the offset alone isn't enough if you're about to hand the pointer
to something that scans for a NUL byte.

---

## 5. `st_shndx` used to index the section array with no bounds check

**File:** `src/x64/x64_utils.c` / `src/x86/x86_utlis.c`, `get_symbol_type_x64/x86()`

```c
Elf64_Shdr *sec = &shdrs[sym->st_shndx];   // st_shndx: attacker-controlled
```

`st_shndx` comes straight from the symbol table and was never checked
against `e_shnum` before indexing the section header array with it.

**Fix:** `if (sym->st_shndx >= ehdr->e_shnum) return '?';` before the
index — the same check is applied wherever a symbol's `st_shndx` is used to
look up a section (both in the type classifier and in the `STT_SECTION`
name-resolution branch in `process_elf64/32`).

**Lesson:** any field read from the file that's later used as an array
index needs the same scrutiny as an offset. It's easy to bounds-check the
"obvious" offsets (into the string table) and miss the ones used as raw
indices.

---

## 6. Addresses were printed in decimal, not hex

**File:** `src/utils/print_utils.c`, `convert_addr()`

```c
char *convert_addr(int addr, int is_64)
{
    char str_64[17];
    ...
    char *nbr = ft_itoa(addr);   // ft_itoa is decimal
    ...
}
```

`ft_itoa` converts to base 10. Every nonzero address in the output was
wrong. Proven directly against real `nm`:

```
real nm:  0000000000003e08 d _DYNAMIC
ft_nm:    0000000000015880 d _DYNAMIC        # 15880 (decimal) == 0x3e08 (hex)
```

This survived the whole original test suite because **both** `tester.sh`
and `diff.sh` stripped the address column before diffing
(`awk '{print $2, $3}'` / `parts[1:3]`) — the tests were only ever
comparing the type letter and the symbol name.

There's a second bug stacked on top: `convert_addr(int addr, ...)` takes a
32-bit `int`, but the real value (`t_header.addr`) is a 64-bit `size_t` —
every call was silently truncating the top 32 bits before the decimal bug
even applied.

**Fix:** a real hex formatter, taking `size_t`:

```c
char *convert_addr(size_t addr, int is_64)
{
    static const char digits[] = "0123456789abcdef";
    int width = is_64 ? 16 : 8;
    char *buf = malloc(width + 1);
    ...
    while (i >= 0) { buf[i] = digits[addr & 0xf]; addr >>= 4; i--; }
    ...
}
```

and `diff.sh`/`tester.sh` were fixed to compare the full
`address type name` triplet.

**Lesson:** a bug can hide in your blind spot indefinitely if your own test
harness has the same blind spot. Whenever a test script does something
like "compare only these columns", ask *why* — sometimes it's a
deliberate, documented exclusion (real `nm`'s filename/ordering quirks),
and sometimes it's quietly eating the thing you actually needed to check.

---

## 7. Address column went blank for defined symbols valued exactly `0`

**File:** `src/utils/print_utils.c`, `print_content()`

```c
if (content->addr ||
    content->type_char == 'T' ||
    content->type_char == 't' ||
    content->type_char == 'a' ||
    content->type_char == 'N' ||
    content->type_char == 'b' ||
    content->type_char == 'D' ||
    content->type_char == 'r')
{
    /* print address */
}
else
{
    /* print blank padding instead */
}
```

This is a positive whitelist of "types that get an address even at 0", and
it's incomplete — `d`, `R`, `A`, `c`, `C` are all missing. Any symbol of
one of those types whose value happens to be exactly `0` (a local `.data`
symbol at the start of the section, for example) lost its address and
printed blank padding instead of `00000000...`, unlike real `nm`.

**Reproduced:** a `.data`-section symbol in a real compiled `.o` file:
`ft_nm` printed `d .data` (blank address), real `nm` printed
`00000000 d .data`.

**Fix:** flip the condition to a blacklist of the two cases that
*shouldn't* get an address (`U` undefined, `w` weak-undefined) instead of
enumerating every case that should:

```c
if (content->type_char != 'U' && content->type_char != 'w')
{
    /* print address, even if it's 0 */
}
```

**Lesson:** when classifying by "everything except a small, well-defined
set", write it as a blacklist of that set. A positive whitelist has to be
kept in sync with every new case that shows up (new symbol types, new
section flags), and it fails silently — you just quietly lose the address
column instead of getting an error.

---

## 8. Multi-file `"filename:"` header logic was inverted

**File:** `src/main.c`, `main()`

```c
if (argc == (flags.total + 1))
{
    if (argc > (flags.total + 1))
        error_counter += ft_nm("a.out", flags, 0);
    else
        error_counter += ft_nm("a.out", flags, 1);   // always taken: header ON
}
else
{
    for (int i = 1; i < argc; i++) {
        if (argc > (flags.total + 1))
            error_counter += ft_nm(argv[i], flags, 0);  // always taken: header OFF
        ...
    }
}
```

Real `nm` prints a `\nfilename:\n` header before each file's symbols *only*
when 2+ files are given — never for a single explicit file, never for the
implicit `a.out` default. The inner `if (argc > (flags.total + 1))` checks
are dead code: inside the outer `if` branch the condition is always false
(so the `a.out` case always takes `multiple = 1`), and inside the outer
`else` branch it's always true (so every multi-file run always takes
`multiple = 0`) — exactly backwards from what's needed.

**Reproduced:** `nm file1.o file2.o` prints a header per file; the original
`ft_nm file1.o file2.o` printed no headers at all, while
`ft_nm` with zero args (falling back to `a.out`) incorrectly printed one.

**Fix:** compute what "multiple" actually means directly instead of
through double-negated arithmetic:

```c
int file_count = argc - 1 - flags.total;
int multiple   = (file_count > 1);
if (file_count <= 0)
    error_counter += ft_nm("a.out", flags, 0);
else
    for (int i = 1; i < argc; i++)
        error_counter += ft_nm(argv[i], flags, multiple);
```

**Lesson:** if a condition needs to be re-derived through two nested
`if`s that both reference the same comparison, that's a sign the boolean
should just be computed once, directly, from what it's supposed to mean.
The original code technically "worked" for the single most common case
(one explicit file → no header) purely by accident, which is exactly why
it went unnoticed.

---

## 9. `open()` on a FIFO/device blocks forever instead of being rejected

**File:** `src/main.c`, `ft_nm()`

The original code called `open(file, O_RDONLY)` unconditionally. Opening a
FIFO for reading blocks until a writer shows up on the other end — so
`ft_nm some_named_pipe` just hangs. Real `nm` checks the file type first
and rejects anything that isn't a regular file with
`nm: Warning: 'x' is not an ordinary file`.

**Reproduced:** `mkfifo /tmp/p && ./ft_nm /tmp/p` hung until killed by
`timeout`.

**Fix:** `stat()` the file *before* opening it, and reject anything that
isn't `S_ISREG` (directories get their own, different warning to match
real `nm`):

```c
if (stat(file, &st) < 0)
    return error(file, "No such file", 1);
if (S_ISDIR(st.st_mode))
    return warn_directory(file);
if (!S_ISREG(st.st_mode))
    return warn_not_ordinary(file);
```

**Lesson:** a hang is arguably worse than a crash for a program that's
graded/tested automatically — a segfault at least terminates immediately.
"Never crash" implicitly includes "never hang", and classifying the file
*before* touching it with a blocking syscall is the fix, not adding a
timeout after the fact.

---

## 10. O(n³) sort + O(n²) list teardown — hangs on a binary with a few thousand symbols

**File:** `src/utils/utils.c` (`sort_list`), `libft/list/listClear.c`

```c
void sort_list(t_list *list)
{
    t_node *tmp1 = list->head;
    for (int i = 0; (size_t)i < list->size; i++)
    {
        t_node *tmp2 = tmp1->next;
        if (tmp2 && ft_strcmp(content1->name, content2->name) < 0)
        {
            list_swap(tmp1, tmp2);
            i = -1;
            tmp1 = list->head;   // restart the ENTIRE scan from the head
            continue;
        }
        tmp1 = tmp2;
    }
}
```

This is a bubble sort that restarts the whole scan from the head after
*every single swap* — worst case closer to O(n³) than the O(n²) a normal
bubble sort would be. On top of that, `list_clear()` removed nodes one at
a time via `list_del()`, which does a linear head-to-node search per
removal — O(n²) just to tear the list down.

**Reproduced:** a binary compiled with 6000 trivial global symbols. Real
`nm` returned in 4ms. `ft_nm` was still running after 30 seconds (killed by
`timeout`).

**Fix:** sort an array of node pointers with `qsort()` (O(n log n)) and
relink, and free the list directly in one O(n) pass instead of routing
through the per-node linear search:

```c
void sort_list(t_list *list)
{
    size_t n = list->size;
    t_node **arr = malloc(sizeof(t_node *) * n);
    /* fill arr by walking the list once */
    qsort(arr, n, sizeof(t_node *), cmp_desc);
    /* relink head/back/next from the sorted array */
}
```

After the fix: 6000 symbols in 0.11s, 20000 symbols in 0.36s, with output
order verified byte-identical to real `nm`.

**Lesson:** "must never crash" also means "must never hang", and algorithmic
complexity is a correctness property, not just a nice-to-have — a project's
own test fixtures are usually tiny (a handful of symbols), so an O(n³)
algorithm can pass every test in the suite and still fail catastrophically
on the first real-world binary someone throws at it. Always sanity-check
against an input an order of magnitude bigger than anything in your test
suite.

---

## If I were starting over from `intra_repo/`

In rough priority order:

1. **Check every syscall's return value the moment it comes back** —
   `open`, `stat`/`fstat`, `mmap`, `malloc`. Not "eventually", immediately.
2. **Treat every offset/size pulled from the file as hostile input**, and
   bounds-check with subtraction (`if (off > limit || size > limit - off)`),
   never addition (`if (off + size > limit)`), because the addition form is
   exactly what overflows.
3. **Test against real binaries early** — `/bin/ls`, `/lib/*/libc.so.6`,
   anything already on the machine — not just your own compiled `.o` test
   fixtures. Stripped binaries are the majority of what exists in the
   wild, and the subject says so explicitly.
4. **Turn on `-Wall -Wextra -Werror -fsanitize=address,undefined` from day
   one** and leave them on until submission. It wouldn't have caught most
   of the bugs above (they're logic bugs, not warnings), but it's free and
   it catches a different, real class of mistakes for zero cost.
5. **Prefer "exclude the well-known small set" over "list every case you
   can think of"** when classifying by type — see #7. A blacklist of two
   things is easier to get right than a whitelist of eight.
6. **Sanity-check your own test scripts**: if a diff/compare script does
   `awk '{print $2,$3}'` or slices a subset of columns, ask why — it might
   be silently hiding the exact bug you'd otherwise have caught (#6).
7. **Test with an input an order of magnitude bigger than your fixtures**
   at least once. A few thousand symbols is nothing for a real binary and
   immediately surfaces algorithmic complexity bugs that small test files
   never will (#10).
