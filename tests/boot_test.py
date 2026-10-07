#!/usr/bin/env python3
"""Boot LiteOS and exercise its user-space shell through the virtual keyboard."""
import argparse
import socket
import subprocess
import tempfile
import time
from pathlib import Path


def wait_for(output: Path, text: str, deadline: float,
             qemu: subprocess.Popen, offset: int = 0) -> str:
    while time.monotonic() < deadline:
        serial = output.read_bytes()[offset:].decode(errors="replace")
        if text in serial:
            return serial
        if qemu.poll() is not None:
            diagnostics = qemu.stderr.read().decode(errors="replace") if qemu.stderr else ""
            raise AssertionError(f"QEMU exited early (status {qemu.returncode}); "
                                 + serial + diagnostics)
        time.sleep(0.05)
    raise AssertionError(f"timed out waiting for {text!r}; serial output:\n"
                         + output.read_bytes()[offset:].decode(errors="replace"))


def press(monitor: socket.socket, command: str) -> None:
    key_names = {" ": "spc", "\n": "ret", "/": "slash",
                 "|": "shift-backslash", ">": "shift-dot"}
    for key in command:
        name = key_names.get(key, key)
        monitor.sendall(f"sendkey {name} 50\n".encode("ascii"))
        time.sleep(0.12)


def run_command(monitor: socket.socket, output: Path, qemu: subprocess.Popen,
                command: str, expected: str = "") -> None:
    offset = output.stat().st_size
    press(monitor, command + "\n")
    serial = wait_for(output, "liteos$ ", time.monotonic() + 15, qemu, offset)
    if expected and expected not in serial:
        raise AssertionError(f"{command!r} did not produce {expected!r}; got:\n{serial}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("iso", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="liteos-test-") as temp:
        directory = Path(temp)
        output = directory / "serial.log"
        output.touch()
        monitor_path = directory / "monitor.sock"
        qemu = subprocess.Popen([
            "qemu-system-x86_64", "-m", "256M", "-cdrom", str(args.iso.resolve()),
            "-display", "none", "-serial", f"file:{output}",
            "-monitor", f"unix:{monitor_path},server=on,wait=off", "-no-reboot",
        ], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        try:
            deadline = time.monotonic() + 30
            boot = wait_for(output, "liteos$ ", deadline, qemu)
            for required in ("heap ok", "initramfs:", "BOOT OK"):
                assert required in boot, f"kernel initialization missing {required!r}:\n{boot}"
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as monitor:
                while not monitor_path.exists() and time.monotonic() < deadline:
                    time.sleep(0.05)
                monitor.settimeout(3)
                monitor.connect(str(monitor_path))
                run_command(monitor, output, qemu, "hello",
                            "\r\nHello from a user-space ELF process!\r\n")
                run_command(monitor, output, qemu, "mkdir smoke")
                run_command(monitor, output, qemu, "cd smoke")
                run_command(monitor, output, qemu, "pwd", "\r\n/smoke\r\n")
                run_command(monitor, output, qemu, "echo first > out")
                run_command(monitor, output, qemu, "echo second >> out")
                run_command(monitor, output, qemu, "cat out",
                            "\r\nfirst\r\nsecond\r\n")
                run_command(monitor, output, qemu, "echo piped | cat", "\r\npiped\r\n")
                run_command(monitor, output, qemu, "rm out")
                run_command(monitor, output, qemu, "cat out", "sh: cannot open: out")
                listing_start = output.stat().st_size
                run_command(monitor, output, qemu, "ls /bin")
                listing = output.read_bytes()[listing_start:].decode(errors="replace")
                assert "\r\nsh\r\n" in listing and "\r\nhello\r\n" in listing, listing
            print("PASS: kernel initialization, ELF, processes, VFS, pipes and shell")
        finally:
            qemu.terminate()
            try:
                qemu.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                qemu.kill()
                qemu.communicate()


if __name__ == "__main__":
    main()
