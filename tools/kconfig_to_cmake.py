#!/usr/bin/env python3
"""Translate a Kconfig .config file into a CMake include file.

Usage: kconfig_to_cmake.py <.config> <output.cmake>
"""
import sys
import re
import os

def delete_old_config(config_path):
    """Deletes old .config file in root"""
    if os.path.exists(config_path):
        os.remove(config_path)
        print(f"Deleted old .config file at {config_path}")
    else:
        print(f"No old .config file at {config_path}")

def main():

    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <.config> <output.cmake>")
        sys.exit(1)

    config_path, output_path = sys.argv[1], sys.argv[2]
    lines_out = []

    with open(config_path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            m = re.match(r'^CONFIG_(\w+)=(.*)$', line)
            if not m:
                continue

            name, value = m.group(1), m.group(2)

            if value == "y":
                lines_out.append(f'set(CONFIG_{name} ON CACHE BOOL "" FORCE)')
            elif value == "n":
                lines_out.append(f'set(CONFIG_{name} OFF CACHE BOOL "" FORCE)')
            else:
                value = value.strip('"')
                lines_out.append(f'set(CONFIG_{name} "{value}" CACHE STRING "" FORCE)')

    with open(output_path, "w") as f:
        f.write("# Auto-generated from .config by tools/kconfig_to_cmake.py\n")
        f.write("# Do not edit directly -- edit .config (via menuconfig) instead\n")
        f.write("\n".join(lines_out) + "\n")

    print(f"Wrote {len(lines_out)} config entries to {output_path}")

    delete_old_config(config_path)


if __name__ == "__main__":
    main()