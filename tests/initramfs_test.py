#!/usr/bin/env python3
"""Validate the archive layout consumed by the kernel's newc loader."""
import sys
from pathlib import Path


def entries(blob: bytes):
    offset = 0
    while offset + 110 <= len(blob):
        assert blob[offset:offset + 6] == b"070701"
        fields = [int(blob[offset + 6 + i * 8:offset + 14 + i * 8], 16)
                  for i in range(13)]
        name_size = fields[11]
        size = fields[6]
        assert name_size > 0
        offset += 110
        name = blob[offset:offset + name_size - 1].decode("ascii")
        assert blob[offset + name_size - 1] == 0
        offset = (offset + name_size + 3) & ~3
        data = blob[offset:offset + size]
        assert len(data) == size
        offset = (offset + size + 3) & ~3
        yield name, fields[1], data
        if name == "TRAILER!!!":
            assert offset == len(blob)
            return
    raise AssertionError("missing cpio trailer")


def main() -> None:
    archive = list(entries(Path(sys.argv[1]).read_bytes()))
    assert [entry[0] for entry in archive] == ["bin", "bin/sh", "bin/hello", "TRAILER!!!"]
    assert archive[0][1] & 0o170000 == 0o040000
    for _, mode, data in archive[1:3]:
        assert mode & 0o170000 == 0o100000
        assert data.startswith(b"\x7fELF")
        entry = int.from_bytes(data[24:32], "little")
        phoff = int.from_bytes(data[32:40], "little")
        phentsize = int.from_bytes(data[54:56], "little")
        phnum = int.from_bytes(data[56:58], "little")
        executable_entry = False
        for i in range(phnum):
            header = data[phoff + i * phentsize:phoff + (i + 1) * phentsize]
            assert len(header) == phentsize
            segment_type = int.from_bytes(header[0:4], "little")
            flags = int.from_bytes(header[4:8], "little")
            address = int.from_bytes(header[16:24], "little")
            length = int.from_bytes(header[40:48], "little")
            executable_entry |= segment_type == 1 and bool(flags & 1) and address <= entry < address + length
        assert executable_entry
    print("PASS: initramfs newc and user ELF files")


if __name__ == "__main__":
    main()
