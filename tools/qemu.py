#!/usr/bin/env python3
"""Launch mexOS image in QEMU with configurable devices."""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path
from typing import Literal, cast

ROOT = Path(__file__).resolve().parent.parent

RunMode = Literal["auto", "iso", "elf"]
NetworkMode = Literal["user", "none"]
UsbMode = Literal["both", "uhci", "ehci", "none"]
SerialMode = Literal["stdio", "none"]


@dataclass(frozen=True)
class Options:
    build_dir: Path
    run_mode: RunMode
    qemu: str
    memory: str
    disk_image: Path | None
    network: NetworkMode
    hostfwd: list[str]
    usb: UsbMode
    usb_storage: Path | None
    serial: SerialMode
    serial_file: Path | None
    headless: bool
    debug_log: Path | None
    qemu_arg: list[str]
    dry_run: bool


def parse_args(argv: Sequence[str] | None = None) -> Options:
    """
    Parse command-line arguments and return an Options object.
    :param argv: The list of command-line arguments to parse. If None, defaults to sys.argv[1:].
    :return: An Options object containing the parsed arguments.
    """
    parser = argparse.ArgumentParser(
        description="Run an existing mexOS build with selected QEMU devices."
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=ROOT / "build",
        help="Build directory containing mexOS.elf and/or mexOS.iso (default: build)",
    )
    parser.add_argument(
        "--run-mode",
        choices=("auto", "iso", "elf"),
        default="auto",
        help="Boot ISO, kernel ELF, or prefer ISO automatically (default: auto)",
    )
    parser.add_argument(
        "--qemu",
        default=os.environ.get("QEMU_SYSTEM_I386", "qemu-system-i386"),
        help="QEMU executable (default: qemu-system-i386)",
    )
    parser.add_argument(
        "--memory",
        default="128M",
        help="Guest memory size accepted by QEMU (default: 128M)",
    )
    parser.add_argument(
        "--disk-image",
        type=Path,
        help="Attach a raw IDE disk image; omitted if not specified",
    )
    parser.add_argument(
        "--network",
        choices=("user", "none"),
        default="user",
        help="Enable an e1000 NIC with user-mode networking, or disable networking",
    )
    parser.add_argument(
        "--hostfwd",
        action="append",
        default=[],
        metavar="RULE",
        help="Add a user-network host forward, e.g. tcp::2222-:22 (repeatable)",
    )
    parser.add_argument(
        "--usb",
        choices=("both", "uhci", "ehci", "none"),
        default="both",
        help="USB host controllers to add (default: both)",
    )
    parser.add_argument(
        "--usb-storage",
        type=Path,
        metavar="IMAGE",
        help="Attach a raw image as a QEMU USB mass-storage device",
    )
    parser.add_argument(
        "--serial",
        choices=("stdio", "none"),
        default="stdio",
        help="Send guest serial output to this terminal, or disable serial (default: stdio)",
    )
    parser.add_argument(
        "--serial-file",
        type=Path,
        help="Write guest serial output to a file instead of --serial",
    )
    parser.add_argument(
        "--headless",
        action="store_true",
        help="Disable the graphical display",
    )
    parser.add_argument(
        "--debug-log",
        type=Path,
        help="Write QEMU CPU/reset/guest-error diagnostics to this file",
    )
    parser.add_argument(
        "--qemu-arg",
        action="append",
        default=[],
        metavar="ARG",
        help="Pass an additional argument to QEMU (repeatable)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the QEMU command without launching it",
    )
    args = parser.parse_args(argv)

    if args.network == "none" and args.hostfwd:
        parser.error("--hostfwd requires --network user")
    if args.usb == "none" and args.usb_storage:
        parser.error("--usb-storage requires a USB controller")
    if args.serial_file and args.serial != "stdio":
        parser.error("--serial-file cannot be combined with --serial none")

    return Options(
        build_dir=args.build_dir,
        run_mode=cast(RunMode, args.run_mode),
        qemu=args.qemu,
        memory=args.memory,
        disk_image=args.disk_image,
        network=cast(NetworkMode, args.network),
        hostfwd=args.hostfwd,
        usb=cast(UsbMode, args.usb),
        usb_storage=args.usb_storage,
        serial=cast(SerialMode, args.serial),
        serial_file=args.serial_file,
        headless=args.headless,
        debug_log=args.debug_log,
        qemu_arg=args.qemu_arg,
        dry_run=args.dry_run,
    )


def require_file(path: Path, description: str) -> Path:
    """
    Resolve an input file or raise a CLI-friendly error.
    :param path: The path to the file to check.
    :param description: The description of the file for error messages.
    :return: The resolved path to the file.
    """
    if not path.is_file():
        raise ValueError(f"{description} does not exist: {path}")
    return path.resolve()


def select_boot_image(build_dir: Path, run_mode: str) -> tuple[str, str]:
    """
    Choose the requested boot image, preferring ISO in auto mode.
    :param build_dir: The build directory containing mexOS.elf and/or mexOS.iso.
    :param run_mode: The run mode, which can be "auto", "iso", or "elf".
    :return: A tuple containing the QEMU argument and the path to the boot image.
    """
    iso = build_dir / "mexOS.iso"
    elf = build_dir / "mexOS.elf"

    if run_mode == "iso":
        return "-cdrom", str(require_file(iso, "ISO image"))
    if run_mode == "elf":
        return "-kernel", str(require_file(elf, "Kernel ELF"))
    if iso.is_file():
        return "-cdrom", str(iso.resolve())
    if elf.is_file():
        return "-kernel", str(elf.resolve())
    raise ValueError(f"No mexOS.iso or mexOS.elf found in {build_dir}")


def build_command(options: Options) -> list[str]:
    """Build the QEMU argv without invoking a shell."""
    executable = shutil.which(options.qemu)
    if not executable:
        raise ValueError(f"QEMU executable not found: {options.qemu}")

    command = [executable]
    command.extend(select_boot_image(options.build_dir.resolve(), options.run_mode))
    command.extend(("-m", options.memory))

    if options.disk_image:
        disk = require_file(options.disk_image, "IDE disk image")
        command.extend(("-drive", f"file={disk},format=raw,if=ide,index=0,media=disk"))

    if options.network == "user":
        netdev = "user,id=net0"
        for rule in options.hostfwd:
            netdev += f",hostfwd={rule}"
        command.extend(("-netdev", netdev, "-device", "e1000,netdev=net0"))

    if options.usb in ("both", "uhci"):
        command.extend(("-device", "piix3-usb-uhci"))
    if options.usb in ("both", "ehci"):
        command.extend(("-device", "usb-ehci"))
    if options.usb_storage:
        image = require_file(options.usb_storage, "USB storage image")
        command.extend(
            (
                "-drive",
                f"file={image},format=raw,if=none,id=usb_disk",
                "-device",
                "usb-storage,drive=usb_disk",
            )
        )

    if options.headless:
        command.extend(("-display", "none"))

    if options.serial_file:
        command.extend(("-serial", f"file:{options.serial_file.resolve()}"))
    elif options.serial == "stdio":
        # Keep the QEMU monitor available in headless mode alongside serial I/O.
        command.extend(("-serial", "mon:stdio" if options.headless else "stdio"))
    else:
        command.extend(("-serial", "none"))

    if options.debug_log:
        command.extend(
            (
                "-d",
                "int,cpu_reset,guest_errors",
                "-D",
                str(options.debug_log.resolve()),
            )
        )

    command.extend(options.qemu_arg)
    return command


def main(argv: Sequence[str] | None = None) -> int:
    """
    Run QEMU or print the command when dry-run is selected.
    :param argv: The list of command-line arguments to parse. If None, defaults to sys.argv[1:].
    :return: The exit code of the QEMU process or 0 if dry-run is selected.
    """
    options = parse_args(argv)
    try:
        command = build_command(options)
    except ValueError as error:
        print(f"qemu.py: error: {error}", file=sys.stderr)
        return 2

    if options.dry_run:
        print(shlex.join(command))
        return 0

    try:
        return subprocess.run(command, cwd=ROOT, check=False).returncode
    except OSError as error:
        print(f"qemu.py: failed to start QEMU: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
