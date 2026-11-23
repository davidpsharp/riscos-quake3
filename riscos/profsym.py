#!/usr/bin/env python3
# RISC OS port, copyright David Sharp 2026, GPL 2
"""Turns the RISC OS sampling profiler's pcs (one hex address a line) into a
flat profile, using `nm -n` output of the unstripped ELF.
    riscos/profsym.py build/syms.txt build/prof.txt [top]"""
import bisect, collections, sys

syms = []
for line in open(sys.argv[1]):
    parts = line.split(None, 2)
    if len(parts) == 3:
        syms.append((int(parts[0], 16), parts[2].strip()))
syms.sort()
addrs = [a for a, _ in syms]
counts = collections.Counter()
total = 0
for line in open(sys.argv[2]):
    pc = int(line, 16)
    i = bisect.bisect_right(addrs, pc) - 1
    counts[syms[i][1] if i >= 0 and pc < 0x2000000 else '(outside the program: OS/modules)'] += 1
    total += 1
top = int(sys.argv[3]) if len(sys.argv) > 3 else 40
print(f'{total} samples')
for name, n in counts.most_common(top):
    print(f'{100.0 * n / total:6.2f}%  {n:6d}  {name}')
