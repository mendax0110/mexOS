#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ONLY=false
RUN_ONLY=false
INSTALL_DEPS=false
BUILD_DOCS=false
RUN_MODE="auto"
MENUCONFIG=false

SERIAL_MODE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --run-only)
            RUN_ONLY=true
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
        --serial)
            SERIAL_MODE=true
            shift
            ;;
        --menuconfig)
            MENUCONFIG=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --build-only         Build the kernel without running QEMU"
            echo "  --run-only           Run existing build artifacts without rebuilding"
            echo "  --install-deps       Attempt to install required dependencies"
            echo "  --docs               Build documentation with Doxygen"
            echo "  --run-mode MODE      MODE = iso | elf | auto (default)"
            echo "  --serial             Headless mode: no QEMU window, interact via this terminal"
            echo "  --menuconfig         Launch 'menuconfig Kconfig' before building"
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

if $BUILD_ONLY && $RUN_ONLY; then
    echo "ERROR: --build-only and --run-only cannot be used together"
    exit 1
fi


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
                sudo apt-get install -y gcc-multilib g++-multilib cmake qemu-system-x86 doxygen graphviz mtools grub-pc-bin xorriso python3-pip
            elif command -v dnf &>/dev/null; then
                sudo dnf install -y gcc gcc-c++ glibc-devel.i686 libgcc.i686 cmake qemu-system-x86 doxygen graphviz mtools grub-pc-bin xorriso python3-pip
            elif command -v pacman &>/dev/null; then
                sudo pacman -Sy --noconfirm lib32-gcc-libs lib32-glibc cmake qemu-system-x86 doxygen graphviz mtools grub-pc-bin xorriso python-pip
            else
                echo "ERROR: Unknown package manager. Please install dependencies manually:"
                echo "  - gcc-multilib (or equivalent 32-bit GCC support)"
                echo "  - cmake"
                echo "  - qemu-system-x86 (optional, for running)"
                echo "  - doxygen (optional, for documentation)"
                echo "  - python3 + pip (for Kconfig support via kconfiglib)"
                exit 1
            fi
            ;;
        macos)
            if ! command -v brew &>/dev/null; then
                echo "ERROR: Homebrew is required on macOS. Install from https://brew.sh"
                exit 1
            fi
            brew install i686-elf-gcc qemu cmake doxygen graphviz mtools xorriso python3
            ;;
        *)
            echo "ERROR: Unsupported operating system"
            exit 1
            ;;
    esac

    echo "Installing kconfiglib (Kconfig support)..."
    pip3 install --break-system-packages kconfiglib || pip3 install --user kconfiglib

    echo "Dependencies installed successfully!"
}

if $INSTALL_DEPS; then
    install_dependencies
fi

check_dependencies()
{
    local missing=()

    if ! $RUN_ONLY; then
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
    fi

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

if ! $RUN_ONLY; then
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

    KCONFIG_FILE="$SCRIPT_DIR/Kconfig"
    DOTCONFIG_FILE="$SCRIPT_DIR/.config"

    if $MENUCONFIG; then
        if ! command -v menuconfig &>/dev/null; then
            echo "ERROR: 'menuconfig' not found. Install kconfiglib:"
            echo "  pip3 install --break-system-packages kconfiglib"
            exit 1
        fi
        echo "Launching menuconfig..."
        (cd "$SCRIPT_DIR" && menuconfig "$KCONFIG_FILE")
    fi

    rm -rf "$SCRIPT_DIR/build"
    mkdir -p "$SCRIPT_DIR/build"

    if [ -f "$DOTCONFIG_FILE" ]; then
        if ! command -v python3 &>/dev/null; then
            echo "WARNING: python3 not found, cannot translate .config -- using CMake defaults"
        else
            echo "Translating .config -> build/kconfig.cmake"
            python3 "$SCRIPT_DIR/tools/kconfig_to_cmake.py" "$DOTCONFIG_FILE" "$SCRIPT_DIR/build/kconfig.cmake"
        fi
    else
        echo "No .config found -- using built-in CMake defaults."
        echo "Run '$0 --menuconfig' to configure the build interactively."
    fi

    if [ "$OS" = "linux" ]; then
        number_of_processors=$(nproc)
    elif [ "$OS" = "macos" ]; then
        number_of_processors=$(sysctl -n hw.ncpu)
    fi

    cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
    cmake --build "$SCRIPT_DIR/build" -j"$number_of_processors"
else
    echo "Run-only selected -- reusing existing build artifacts."
    if [ ! -d "$SCRIPT_DIR/build" ]; then
        echo "ERROR: build directory does not exist. Run '$0 --build-only' first."
        exit 1
    fi
fi


DISK_IMG="${MEXOS_DISK_IMG:-$SCRIPT_DIR/mexOS.img}"
if ! $BUILD_ONLY; then
    if [ ! -f "$DISK_IMG" ]; then
        echo "Creating blank 64MB disk image..."
        qemu-img create -f raw "$DISK_IMG" 64M
    fi
    echo "Disk image: $DISK_IMG"
fi

echo ""
if [ -f "$SCRIPT_DIR/build/mexOS.elf" ]; then
    echo "Kernel ELF: $SCRIPT_DIR/build/mexOS.elf"
    echo "Kernel build completed successfully!"
fi

if [ -f "$SCRIPT_DIR/build/mexOS.iso" ]; then
    echo "Bootable ISO: $SCRIPT_DIR/build/mexOS.iso"
    echo "ISO build completed successfully!"
fi

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

    QEMU_DISPLAY_FLAGS="-serial stdio"

    if $SERIAL_MODE; then
        QEMU_DISPLAY_FLAGS="-nographic"
        QEMU_COMMON_FLAGS="-m 128M -drive file=$DISK_IMG,format=raw,if=ide,index=0,media=disk -net nic,model=e1000 -net user"
    else
        QEMU_DISPLAY_FLAGS="-serial stdio"
        QEMU_COMMON_FLAGS="-m 128M -drive file=$DISK_IMG,format=raw,if=ide,index=0,media=disk -net nic,model=e1000 -net user -vga std"
    fi

    QEMU_LOG_FLAGS="-d int,cpu_reset,guest_errors -D $SCRIPT_DIR/build/qemu.log"

    case "$RUN_MODE" in
        iso)
            if [ -f "$SCRIPT_DIR/build/mexOS.iso" ]; then
                echo "Starting QEMU with ISO (forced)..."
                qemu-system-i386 -cdrom "$SCRIPT_DIR/build/mexOS.iso" $QEMU_DISPLAY_FLAGS $QEMU_COMMON_FLAGS $QEMU_LOG_FLAGS
            else
                echo "ERROR: --run-mode iso selected, but mexOS.iso does not exist"
                exit 1
            fi
            ;;

        elf)
            if [ -f "$SCRIPT_DIR/build/mexOS.elf" ]; then
                echo "Starting QEMU with ELF (forced)..."
                qemu-system-i386 -kernel "$SCRIPT_DIR/build/mexOS.elf" $QEMU_DISPLAY_FLAGS $QEMU_COMMON_FLAGS $QEMU_LOG_FLAGS
            else
                echo "ERROR: --run-mode elf selected, but mexOS.elf does not exist"
                exit 1
            fi
            ;;

        auto)
            if [ -f "$SCRIPT_DIR/build/mexOS.iso" ]; then
                echo "Starting QEMU with ISO..."
                qemu-system-i386 -cdrom "$SCRIPT_DIR/build/mexOS.iso" $QEMU_DISPLAY_FLAGS $QEMU_COMMON_FLAGS $QEMU_LOG_FLAGS
            elif [ -f "$SCRIPT_DIR/build/mexOS.elf" ]; then
                echo "Starting QEMU with kernel directly..."
                qemu-system-i386 -kernel "$SCRIPT_DIR/build/mexOS.elf" $QEMU_DISPLAY_FLAGS $QEMU_COMMON_FLAGS $QEMU_LOG_FLAGS
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
