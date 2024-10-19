#!/usr/bin/env python3

import sys

print("#include \"types.h\"\n")

f = open(sys.argv[1], "rb")
data = bytearray(f.read())
print("u32 g_boot_code_size = 0x{:x};".format(len(data)))
print("u8 g_boot_code[] = {\n    ", end="")

for i, b in enumerate(data):
    if i == (len(data) - 1):
        print("0x{:02x}".format(b))
        break
    else:
        print("0x{:02x}, ".format(b), end='')

    if ((i + 1) % 8) == 0:
        print("\n    ", end="")

f.close()
print("};\n")