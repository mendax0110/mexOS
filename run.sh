#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ONLY=false
INSTALL_DEPS=false
BUILD_DOCS=false
RUN_MODE="auto"

while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --install-deps)
            INSTALL_DEPS=true
            shift
            ;;
        --docs)
            BUILD_DOCS=true
            shift
            ;;
        --run-mode)
            RUN_MODE="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --build-only         Build the kernel without running QEMU"
            echo "  --install-deps       Attempt to install required dependencies"
            echo "  --run-mode (iso/elf) Set the run mode"
            echo "  --docs               Build documentation with Doxygen"
            echo "  --run-mode MODE      MODE = iso | elf | auto (default)"
            echo "  --help               Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done


detect_os()
{
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        Darwin*)    echo "macos";;
        *)          echo "unknown";;
    esac
}

OS=$(detect_os)
echo "Detected OS: $OS"

install_dependencies()
{
    echo "Installing dependencies..."
    case "$OS" in
        linux)
            if command -v apt-get &>/dev/null; then
                sudo apt-get update
                sudo apt-get install -y gcc-multilib g++-multilib cmake qemu-system-x86 doxygen graphviz mtools grub-pc-bin
            elif command -v dnf &>/dev/null; then
                sudo dnf install -y gcc gcc-c++ glibc-devel.i686 libgcc.i686 cmake qemu-system-x86 doxygen graphviz mtools grub-pc-bin
            elif command -v pacman &>/dev/null; then
                sudo pacman -Sy --noconfirm lib32-gcc-libs lib32-glibc cmake qemu-system-x86 doxygen graphviz mtools grub-pc-bin
            else
                echo "ERROR: Unknown package manager. Please install dependencies manually:"
                echo "  - gcc-multilib (or equivalent 32-bit GCC support)"
                echo "  - cmake"
                echo "  - qemu-system-x86 (optional, for running)"
                echo "  - doxygen (optional, for documentation)"
                exit 1
            fi
            ;;
        macos)
            if ! command -v brew &>/dev/null; then
                echo "ERROR: Homebrew is required on macOS. Install from https://brew.sh"
                exit 1
            fi
            brew install i686-elf-gcc qemu cmake doxygen graphviz mtools
            ;;
        *)
            echo "ERROR: Unsupported operating system"
            exit 1
            ;;
    esac
    echo "Dependencies installed successfully!"
}

if $INSTALL_DEPS; then
    install_dependencies
fi

check_dependencies()
{
    local missing=()

    if ! command -v cmake &>/dev/null; then
        missing+=("cmake")
    fi

    case "$OS" in
        linux)
            if ! gcc -m32 -E -x c /dev/null &>/dev/null; then
                missing+=("gcc-multilib (32-bit GCC support)")
            fi
            ;;
        macos)
            if ! command -v i686-elf-gcc &>/dev/null; then
                missing+=("i686-elf-gcc (install via: brew install i686-elf-gcc)")
            fi
            ;;
    esac

    if ! $BUILD_ONLY; then
        if ! command -v qemu-system-i386 &>/dev/null; then
            missing+=("qemu-system-x86 (optional, required for running)")
        fi
    fi

    if [ ${#missing[@]} -ne 0 ]; then
        echo "ERROR: Missing required dependencies:"
        for dep in "${missing[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo "Run '$0 --install-deps' to attempt automatic installation"
        exit 1
    fi
}

check_dependencies

# Select the appropriate toolchain file
case "$OS" in
    linux)
        TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain.cmake"
        ;;
    macos)
        TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain-macos.cmake"
        ;;
    *)
        echo "ERROR: Unsupported operating system"
        exit 1
        ;;
esac

echo "Using toolchain: $TOOLCHAIN_FILE"

# Build the kernel
rm -rf "$SCRIPT_DIR/build"
mkdir -p "$SCRIPT_DIR/build"
cd "$SCRIPT_DIR/build"

cmake -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" ..
make

echo ""
echo "Build completed successfully!"
echo "Kernel: $SCRIPT_DIR/build/mexOS.elf"
echo "ISO:    $SCRIPT_DIR/build/mexOS.iso"

# Build documentation
if $BUILD_DOCS; then
    echo ""
    echo "Building documentation..."
    cd "$SCRIPT_DIR"
    if command -v doxygen &>/dev/null; then
        doxygen Doxyfile
        echo "Documentation generated in: $SCRIPT_DIR/docs/html"
    else
        echo "Warning: doxygen not found, skipping documentation build"
        echo "Install doxygen to build documentation"
    fi
fi

if ! $BUILD_ONLY; then
    echo ""

    case "$RUN_MODE" in
        iso)
            if [ -f "$SCRIPT_DIR/build/mexOS.iso" ]; then
                echo "Starting QEMU with ISO (forced)..."
                qemu-system-i386 -cdrom "$SCRIPT_DIR/build/mexOS.iso" -serial stdio -m 128M
            else
                echo "ERROR: --run-mode iso selected, but mexOS.iso does not exist"
                exit 1
            fi
            ;;

        elf)
            if [ -f "$SCRIPT_DIR/build/mexOS.elf" ]; then
                echo "Starting QEMU with ELF (forced)..."
                qemu-system-i386 -kernel "$SCRIPT_DIR/build/mexOS.elf" -serial stdio -m 128M
            else
                echo "ERROR: --run-mode elf selected, but mexOS.elf does not exist"
                exit 1
            fi
            ;;

        auto)
            if [ -f "$SCRIPT_DIR/build/mexOS.iso" ]; then
                echo "Starting QEMU with ISO..."
                qemu-system-i386 -cdrom "$SCRIPT_DIR/build/mexOS.iso" -serial stdio -m 128M
            elif [ -f "$SCRIPT_DIR/build/mexOS.elf" ]; then
                echo "Starting QEMU with kernel directly..."
                qemu-system-i386 -kernel "$SCRIPT_DIR/build/mexOS.elf" -serial stdio -m 128M
            else
                echo "Error: No bootable files found"
                exit 1
            fi
            ;;

        *)
            echo "ERROR: Invalid --run-mode value: $RUN_MODE"
            echo "Valid values: iso, elf, auto"
            exit 1
            ;;
    esac
fi

