#!/usr/bin/env python3
"""Create a reproducible newc archive from explicitly named user-space binaries."""
import argparse
from pathlib import Path


def pad(stream):
    stream.write(b"\0" * (-stream.tell() % 4))


def record(stream, name: str, mode: int, data: bytes, inode: int) -> None:
    encoded = name.encode("ascii") + b"\0"
    fields = (inode, mode, 0, 0, 1, 0, len(data), 0, 0, 0, 0, len(encoded), 0)
    stream.write(b"070701" + b"".join(f"{value:08x}".encode("ascii") for value in fields))
    stream.write(encoded)
    pad(stream)
    stream.write(data)
    pad(stream)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("sh", type=Path)
    parser.add_argument("hello", type=Path)
    args = parser.parse_args()
    args.archive.parent.mkdir(parents=True, exist_ok=True)
    with args.archive.open("wb") as archive:
        record(archive, "bin", 0o040755, b"", 1)
        record(archive, "bin/sh", 0o100755, args.sh.read_bytes(), 2)
        record(archive, "bin/hello", 0o100755, args.hello.read_bytes(), 3)
        record(archive, "TRAILER!!!", 0, b"", 4)


if __name__ == "__main__":
    main()
