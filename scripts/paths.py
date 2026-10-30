#!/usr/bin/env python3
"""
Paths over the marked branches (see -write-unfinished-paths), for the tests:

  paths.py mark FILE.ll   print the module with the conditional branches
                          marked (!bbk.branch, numbered from 0 in the order
                          of the file; bubaak marks them by its LLVM pass)
                          and the selects (!bbk.select, numbered after them)
  paths.py show FILE...   print .path files (`ID:DIR ...`) or lists of
                          paths (`TAG ID:DIR ...` per path)
  paths.py list           read lines `TAG ID:DIR ...` and write a list of
                          paths (for -prefix-paths) to the standard output
"""

import re
import sys

PATH_MAGIC = b"BBKP\x01"
LIST_MAGIC = b"BBKR\x01"


def varint(v):
    out = bytearray()
    while v >= 0x80:
        out.append((v & 0x7F) | 0x80)
        v >>= 7
    out.append(v)
    return bytes(out)


def read_varints(data):
    vals, v, shift = [], 0, 0
    for b in data:
        v |= (b & 0x7F) << shift
        shift += 7
        if not b & 0x80:
            vals.append(v)
            v, shift = 0, 0
    return vals


def mark(path):
    with open(path) as f:
        lines = [line.rstrip("\n") for line in f]
    n = 0
    for i, line in enumerate(lines):
        m = re.match(r"\s*br i1 .*, label (%[^,\s]+), label (%[^,\s]+)", line)
        if m:
            lines[i] = line[: m.end()] + f", !bbk.branch !{{i32 {n}}}" + line[m.end() :]
            n += 1
    # (the selects get the IDs after the branches, as in bubaak)
    for i, line in enumerate(lines):
        m = re.match(r"\s*%\S+ = select i1 [^,]+, [^,]+, [^,]+?(?=, !|$)", line)
        if m:
            lines[i] = line[: m.end()] + f", !bbk.select !{{i32 {n}}}" + line[m.end() :]
            n += 1
    print("\n".join(lines))


def entries(vals):
    return " ".join(f"{vals[i]}:{vals[i + 1]}" for i in range(0, len(vals) - 1, 2))


def show(path):
    with open(path, "rb") as f:
        data = f.read()
    if data.startswith(PATH_MAGIC):
        print(entries(read_varints(data[len(PATH_MAGIC) :])))
        return
    assert data.startswith(LIST_MAGIC), "not a path or a list of paths"
    data, pos = data[len(LIST_MAGIC) :], 0

    def next_varint():
        nonlocal pos
        v, shift = 0, 0
        while True:
            b = data[pos]
            pos += 1
            v |= (b & 0x7F) << shift
            shift += 7
            if not b & 0x80:
                return v

    while pos < len(data):
        tag = data[pos + 1 : pos + 1 + data[pos]].decode()
        pos += 1 + data[pos]
        n = next_varint()
        vals = [next_varint() for _ in range(2 * n)]
        print(f"{tag} {entries(vals)}".rstrip() if tag else entries(vals))


def write_list():
    out = bytearray(LIST_MAGIC)
    for line in sys.stdin:
        tag, *toks = line.split()
        out += varint(len(tag)) + tag.encode() + varint(len(toks))
        for tok in toks:
            i, d = tok.split(":")
            out += varint(int(i)) + varint(int(d))
    sys.stdout.buffer.write(out)


if __name__ == "__main__":
    cmd = sys.argv[1]
    if cmd == "mark":
        mark(sys.argv[2])
    elif cmd == "show":
        for path in sys.argv[2:]:
            show(path)
    elif cmd == "list":
        write_list()
    else:
        sys.exit(f"unknown command {cmd}")
