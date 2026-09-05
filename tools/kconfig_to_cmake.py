#!/usr/bin/env python3
"""
Translate a Kconfig .config file into a CMake include file.
Usage: kconfig_to_cmake.py <.config> <output.cmake>
"""

import argparse
import logging
import os
import re
import sys

logging.basicConfig(level=logging.INFO, format="%(message)s")
log = logging.getLogger(__name__)

CONFIG_LINE_RE = re.compile(r"^CONFIG_(\w+)=(.*)$")
CONFIG_PREFIX_RE = re.compile(r"^CONFIG_(\w+)")


def unescape_kconfig_string(value: str) -> str:
    """
    Strip surrounding quotes from a Kconfig string value and unescape it.
    :param value: The Kconfig string value to unescape.
    :return: The unescaped string.
    """
    if value.startswith('"') and value.endswith('"') and len(value) >= 2:
        value = value[1:-1]

    value = value.replace('\\"', '"').replace("\\\\", "\\")
    return value


def escape_cmake_string(value: str) -> str:
    """
    Escape a string so it is safe to embed inside CMake double quotes.
    :param value: The string to escape.
    :return: The escaped string.
    """
    value = value.replace("\\", "\\\\")
    value = value.replace('"', '\\"')
    value = value.replace(";", "\\;")
    return value


def delete_old_config(config_path: str) -> None:
    """
    Delete the old .config file.
    :param config_path: The path to the .config file to delete.
    """
    if os.path.exists(config_path):
        os.remove(config_path)
        log.info(f"Deleted old .config file at {config_path}")
    else:
        log.info(f"No old .config file at {config_path}")


def parse_config(config_path: str) -> list[str]:
    """
    Parse the given Kconfig .config file into CMake set() statements.
    :param config_path: The path to the Kconfig .config file.
    :return: A list of CMake set() statements as strings.
    """
    lines_out = []
    seen_names = set()

    try:
        with open(config_path, encoding="utf-8") as f:
            raw_lines = f.readlines()
    except OSError as e:
        log.error(f"Error: could not read '{config_path}': {e}")
        sys.exit(1)

    for lineno, line in enumerate(raw_lines, start=1):
        line = line.strip()

        if not line or line.startswith("#"):
            continue

        match = CONFIG_LINE_RE.match(line)

        if not match:
            if CONFIG_PREFIX_RE.match(line):
                log.warning(f"Warning: {config_path}:{lineno}: " f"malformed CONFIG line, skipping: {line}")
            continue

        name, value = match.group(1), match.group(2)

        if name in seen_names:
            log.warning(f"Warning: {config_path}:{lineno}: " f"duplicate CONFIG_{name} entry, skipping")
            continue

        seen_names.add(name)

        if value == "y":
            lines_out.append(f'set(CONFIG_{name} ON CACHE BOOL "" FORCE)')
        elif value == "n":
            lines_out.append(f'set(CONFIG_{name} OFF CACHE BOOL "" FORCE)')
        else:
            value = unescape_kconfig_string(value)
            value = escape_cmake_string(value)
            lines_out.append(f'set(CONFIG_{name} "{value}" CACHE STRING "" FORCE)')

    return lines_out


def write_cmake(output_path: str, lines_out: list[str]) -> None:
    """
    Write the generated CMake lines to the output file.
    :param output_path: The path to the output CMake file.
    :param lines_out: The list of CMake set() statements to write.
    """
    try:
        with open(output_path, "w", encoding="utf-8") as f:
            f.write("# Auto-generated from .config by tools/kconfig_to_cmake.py\n")
            f.write("# Do not edit directly -- edit .config (via menuconfig) instead\n")
            f.write("\n".join(lines_out) + "\n")
    except OSError as e:
        log.error(f"Error: could not write '{output_path}': {e}")
        sys.exit(1)


def main() -> None:
    """
    Main function to parse command-line arguments and perform the translation from Kconfig .config to CMake.
    :return: None
    """
    parser = argparse.ArgumentParser(description="Translate a Kconfig .config file into a CMake include file.")
    parser.add_argument("config_path", metavar=".config", help="Path to the Kconfig .config file",)
    parser.add_argument("output_path", metavar="output.cmake", help="Path to write the generated CMake file",)
    args = parser.parse_args()

    lines_out = parse_config(args.config_path)
    write_cmake(args.output_path, lines_out)

    log.info(f"Successfully generated CMake file at " f"{args.output_path} from {args.config_path}")

    delete_old_config(args.config_path)


if __name__ == "__main__":
    main()