#!/bin/bash

set -e

for tool in gcc g++ as ld; do
    if ! command -v i686-elf-$tool &>/dev/null; then
        echo "ERROR: i686-elf-$tool not found"
        exit 1
    fi
done

rm -rf build
mkdir -p build
cd build

cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make

if [ -f mexOS.iso ]; then
    echo "Starting QEMU with ISO..."
    qemu-system-i386 -cdrom mexOS.iso -serial stdio -m 512M -d cpu_reset
elif [ -f mexOS.elf ]; then
    echo "Starting QEMU with kernel directly..."
    qemu-system-i386 -kernel mexOS.elf -serial stdio -m 512M -d cpu_reset
else
    echo "Error: No bootable files found"
    exit 1
fi