#!/usr/bin/env python3
"""Generate book_data.cpp from book.bin — fast, no CMake string loops."""
import sys, os

book_path = sys.argv[1] if len(sys.argv) > 1 else "book.bin"
out_path = sys.argv[2] if len(sys.argv) > 2 else "book_data.cpp"

data = open(book_path, "rb").read()
print(f"Embedding {len(data)} bytes ({len(data)//16} entries)")

lines = ["#include <cstddef>", "extern const unsigned char book_data[] = {"]
for i in range(0, len(data), 16):
    chunk = data[i:i+16]
    hex_bytes = ",".join(f"0x{b:02x}" for b in chunk)
    lines.append(f"  {hex_bytes},")
lines.append("};")
lines.append(f"extern const size_t book_data_size = {len(data)};")

with open(out_path, "w") as f:
    f.write("\n".join(lines) + "\n")

print(f"Wrote {out_path}")
