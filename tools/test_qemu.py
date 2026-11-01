#!/usr/bin/env python3
"""Boot mexOS under QEMU with several network and USB device profiles."""

from __future__ import annotations

import shlex
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BOOT_MARKER = "[boot] Boot complete!"
BOOT_TIMEOUT_SECONDS = 20
USB_IMAGE_SIZE = 1024 * 1024


@dataclass(frozen=True)
class Profile:
    name: str
    options: tuple[str, ...]
    required_args: tuple[str, ...]
    forbidden_args: tuple[str, ...] = ()


PROFILES = (
    Profile(
        "network + UHCI + EHCI + USB storage",
        (
            "--network",
            "user",
            "--usb",
            "both",
            "--usb-storage",
        ),
        (
            "user,id=net0",
            "e1000,netdev=net0",
            "piix3-usb-uhci",
            "usb-ehci",
            "usb-storage,drive=usb_disk",
        ),
    ),
    Profile(
        "UHCI only",
        ("--network", "none", "--usb", "uhci"),
        ("piix3-usb-uhci",),
        ("-netdev", "usb-ehci", "usb-storage,drive=usb_disk"),
    ),
    Profile(
        "EHCI only",
        ("--network", "none", "--usb", "ehci"),
        ("usb-ehci",),
        ("-netdev", "piix3-usb-uhci", "usb-storage,drive=usb_disk"),
    ),
    Profile(
        "no network or USB",
        ("--network", "none", "--usb", "none"),
        (),
        ("-netdev", "-device"),
    ),
)


def base_command(serial_log: Path, debug_log: Path) -> list[str]:
    """Return shared launcher options for a profile."""
    return [
        sys.executable,
        str(ROOT / "tools" / "qemu.py"),
        "--run-mode",
        "elf",
        "--headless",
        "--serial-file",
        str(serial_log),
        "--debug-log",
        str(debug_log),
        "--qemu-arg=-no-reboot",
        "--qemu-arg=-no-shutdown",
    ]


def boot_profile(profile: Profile, temp: Path, usb_image: Path) -> bool:
    """Validate the generated QEMU arguments, then boot to completion."""
    serial_log = temp / "serial.log"
    debug_log = temp / "qemu-debug.log"
    stdout_log = temp / "qemu-stdout.log"
    stderr_log = temp / "qemu-stderr.log"
    serial_log.unlink(missing_ok=True)

    options = list(profile.options)
    if "--usb-storage" in options:
        options.append(str(usb_image))

    dry_command = base_command(serial_log, debug_log) + options + ["--dry-run"]
    try:
        dry_result = subprocess.run(
            dry_command,
            cwd=ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as error:
        print(
            f"QEMU helper failed for profile '{profile.name}': {error}", file=sys.stderr
        )
        return False

    if dry_result.returncode != 0:
        print(dry_result.stderr, file=sys.stderr)
        print(f"QEMU command generation failed: {profile.name}", file=sys.stderr)
        return False

    generated_args = shlex.split(dry_result.stdout)
    missing_args = [arg for arg in profile.required_args if arg not in generated_args]
    unexpected_args = [arg for arg in profile.forbidden_args if arg in generated_args]
    if missing_args or unexpected_args:
        print(f"QEMU command profile failed: {profile.name}", file=sys.stderr)
        if missing_args:
            print(f"Missing arguments: {', '.join(missing_args)}", file=sys.stderr)
        if unexpected_args:
            print(
                f"Unexpected arguments: {', '.join(unexpected_args)}", file=sys.stderr
            )
        return False

    command = base_command(serial_log, debug_log) + options
    process: subprocess.Popen[bytes] | None = None
    try:
        with (
            stdout_log.open("w", encoding="utf-8") as stdout_file,
            stderr_log.open("w", encoding="utf-8") as stderr_file,
        ):
            process = subprocess.Popen(
                command,
                cwd=ROOT,
                stdout=stdout_file,
                stderr=stderr_file,
            )
            deadline = time.monotonic() + BOOT_TIMEOUT_SECONDS
            while time.monotonic() < deadline:
                if serial_log.exists():
                    output = serial_log.read_text(encoding="utf-8", errors="replace")
                    if BOOT_MARKER in output:
                        break
                if process.poll() is not None:
                    break
                time.sleep(0.1)
            else:
                output = ""
    except OSError as error:
        print(f"QEMU failed for profile '{profile.name}': {error}", file=sys.stderr)
        return False
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

    if serial_log.exists():
        output = serial_log.read_text(encoding="utf-8", errors="replace")
    else:
        output = ""

    if BOOT_MARKER not in output:
        print(f"QEMU boot failed: {profile.name}", file=sys.stderr)
        print(f"Missing serial marker: {BOOT_MARKER}", file=sys.stderr)
        print(output[-4000:], file=sys.stderr)
        if debug_log.exists():
            print(
                debug_log.read_text(encoding="utf-8", errors="replace")[-2000:],
                file=sys.stderr,
            )
        for log in (stdout_log, stderr_log):
            if log.exists():
                print(
                    log.read_text(encoding="utf-8", errors="replace")[-1000:],
                    file=sys.stderr,
                )
        return False

    print(f"QEMU profile passed: {profile.name}")
    return True


def main() -> int:
    """Run the device-profile smoke tests against the built kernel."""
    if not (ROOT / "build" / "mexOS.elf").is_file():
        print(
            "Missing build/mexOS.elf; build the kernel before testing.", file=sys.stderr
        )
        return 2

    failures = 0
    with tempfile.TemporaryDirectory(prefix="mexos-qemu-test-") as temp_dir:
        temp = Path(temp_dir)
        usb_image = temp / "usb-test.img"
        with usb_image.open("wb") as image_file:
            image_file.truncate(USB_IMAGE_SIZE)

        for index, profile in enumerate(PROFILES):
            profile_dir = temp / f"profile-{index}"
            profile_dir.mkdir()
            if not boot_profile(profile, profile_dir, usb_image):
                failures += 1

    if failures:
        print(f"{failures} QEMU profile(s) failed.", file=sys.stderr)
        return 1

    print(f"All {len(PROFILES)} QEMU device profiles passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
