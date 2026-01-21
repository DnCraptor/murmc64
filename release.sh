#!/bin/bash
# Copyright (c) 2024-2026 Mikhail Matveev <xtreme@rh1.tech>
#
# release.sh - Build all release variants of MurmC64
#
# Creates firmware files for each board variant (M1, M2) at each clock speed:
#   - Non-overclocked: 252 MHz CPU, 100 MHz PSRAM
#   - Medium overclock: 378 MHz CPU, 133 MHz PSRAM
#   - Max overclock: 504 MHz CPU, 166 MHz PSRAM
#
# Output formats:
#   - UF2 files for direct flashing via BOOTSEL mode
#   - m1p2/m2p2 files for Murmulator OS
#
# All builds include USB HID support (keyboard/mouse via USB).
# PS/2 keyboard and NES gamepad are also supported simultaneously.
# USB Serial debug output is DISABLED in release builds.
#
# Output format: murmc64_mX_Y_Z_A_BB.{uf2,m1p2,m2p2}
#   X  = Board variant (1 or 2)
#   Y  = CPU clock in MHz
#   Z  = PSRAM clock in MHz (target)
#   A  = Major version
#   BB = Minor version (zero-padded)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Version file
VERSION_FILE="version.txt"

# Read last version or initialize
if [[ -f "$VERSION_FILE" ]]; then
    read -r LAST_MAJOR LAST_MINOR < "$VERSION_FILE"
else
    LAST_MAJOR=1
    LAST_MINOR=0
fi

# Calculate next version (for default suggestion)
NEXT_MINOR=$((LAST_MINOR + 1))
NEXT_MAJOR=$LAST_MAJOR
if [[ $NEXT_MINOR -ge 100 ]]; then
    NEXT_MAJOR=$((NEXT_MAJOR + 1))
    NEXT_MINOR=0
fi

# Interactive version input
echo ""
echo -e "${CYAN}┌─────────────────────────────────────────────────────────────────┐${NC}"
echo -e "${CYAN}│                    MurmC64 Release Builder                      │${NC}"
echo -e "${CYAN}└─────────────────────────────────────────────────────────────────┘${NC}"
echo ""
echo -e "Last version: ${YELLOW}${LAST_MAJOR}.$(printf '%02d' $LAST_MINOR)${NC}"
echo ""

DEFAULT_VERSION="${NEXT_MAJOR}.$(printf '%02d' $NEXT_MINOR)"
read -p "Enter version [default: $DEFAULT_VERSION]: " INPUT_VERSION
INPUT_VERSION=${INPUT_VERSION:-$DEFAULT_VERSION}

# Parse version (handle both "1.00" and "1 00" formats)
if [[ "$INPUT_VERSION" == *"."* ]]; then
    MAJOR="${INPUT_VERSION%%.*}"
    MINOR="${INPUT_VERSION##*.}"
else
    read -r MAJOR MINOR <<< "$INPUT_VERSION"
fi

# Remove leading zeros for arithmetic, then re-pad
MINOR=$((10#$MINOR))
MAJOR=$((10#$MAJOR))

# Validate
if [[ $MAJOR -lt 1 ]]; then
    echo -e "${RED}Error: Major version must be >= 1${NC}"
    exit 1
fi
if [[ $MINOR -lt 0 || $MINOR -ge 100 ]]; then
    echo -e "${RED}Error: Minor version must be 0-99${NC}"
    exit 1
fi

# Format version string
VERSION="${MAJOR}_$(printf '%02d' $MINOR)"
VERSION_DOT="${MAJOR}.$(printf '%02d' $MINOR)"
echo ""
echo -e "${GREEN}Building release version: ${VERSION_DOT}${NC}"

# Save new version
echo "$MAJOR $MINOR" > "$VERSION_FILE"

# Create release directory
RELEASE_DIR="$SCRIPT_DIR/release"
mkdir -p "$RELEASE_DIR"

# Build configurations: "BOARD CPU_SPEED PSRAM_SPEED DESCRIPTION"
CONFIGS=(
    "M1 252 100 non-overclocked"
    "M1 378 133 medium-overclock"
    "M1 504 166 max-overclock"
    "M2 252 100 non-overclocked"
    "M2 378 133 medium-overclock"
    "M2 504 166 max-overclock"
)

BUILD_COUNT=0
# Total builds: UF2 + MOS2 versions (2x configs)
TOTAL_BUILDS=$((${#CONFIGS[@]} * 2))

echo ""
echo -e "${YELLOW}Building $TOTAL_BUILDS firmware variants (UF2 + MOS2)...${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

echo ""
echo -e "${CYAN}=== Building UF2 firmware files ===${NC}"

for config in "${CONFIGS[@]}"; do
    read -r BOARD CPU PSRAM DESC <<< "$config"

    BUILD_COUNT=$((BUILD_COUNT + 1))

    # Board variant number
    if [[ "$BOARD" == "M1" ]]; then
        BOARD_NUM=1
    else
        BOARD_NUM=2
    fi

    # Output filename
    OUTPUT_NAME="murmc64_m${BOARD_NUM}_${CPU}_${PSRAM}_${VERSION}.uf2"

    echo ""
    echo -e "${CYAN}[$BUILD_COUNT/$TOTAL_BUILDS] Building: $OUTPUT_NAME${NC}"
    echo -e "  Board: $BOARD | CPU: ${CPU} MHz | PSRAM: ${PSRAM} MHz | $DESC"

    # Clean and create build directory
    rm -rf build
    mkdir build
    cd build

    # Configure with CMake
    # Release builds: USB_HID_ENABLED=1, DEBUG_LOGS_ENABLED=OFF
    cmake .. \
        -DPICO_PLATFORM=rp2350 \
        -DBOARD_VARIANT="$BOARD" \
        -DCPU_SPEED="$CPU" \
        -DPSRAM_SPEED="$PSRAM" \
        -DUSB_HID_ENABLED=1 \
        -DDEBUG_LOGS_ENABLED=OFF \
        -DFIRMWARE_VERSION="v${VERSION_DOT}" \
        > /dev/null 2>&1

    # Build
    if make -j8 > /dev/null 2>&1; then
        # Copy UF2 to release directory
        if [[ -f "murmc64.uf2" ]]; then
            cp "murmc64.uf2" "$RELEASE_DIR/$OUTPUT_NAME"
            echo -e "  ${GREEN}✓ Success${NC} → release/$OUTPUT_NAME"
        else
            echo -e "  ${RED}✗ UF2 not found${NC}"
        fi
    else
        echo -e "  ${RED}✗ Build failed${NC}"
    fi

    cd "$SCRIPT_DIR"
done

echo ""
echo -e "${CYAN}=== Building MOS2 firmware files (Murmulator OS) ===${NC}"

for config in "${CONFIGS[@]}"; do
    read -r BOARD CPU PSRAM DESC <<< "$config"

    BUILD_COUNT=$((BUILD_COUNT + 1))

    # Board variant number and MOS2 extension
    if [[ "$BOARD" == "M1" ]]; then
        BOARD_NUM=1
        MOS2_EXT="m1p2"
    else
        BOARD_NUM=2
        MOS2_EXT="m2p2"
    fi

    # Output filename for MOS2
    OUTPUT_NAME="murmc64_m${BOARD_NUM}_${CPU}_${PSRAM}_${VERSION}.${MOS2_EXT}"

    echo ""
    echo -e "${CYAN}[$BUILD_COUNT/$TOTAL_BUILDS] Building: $OUTPUT_NAME${NC}"
    echo -e "  Board: $BOARD | CPU: ${CPU} MHz | PSRAM: ${PSRAM} MHz | $DESC | MOS2"

    # Clean and create build directory
    rm -rf build
    mkdir build
    cd build

    # Configure with CMake (MOS2 enabled)
    cmake .. \
        -DPICO_PLATFORM=rp2350 \
        -DBOARD_VARIANT="$BOARD" \
        -DCPU_SPEED="$CPU" \
        -DPSRAM_SPEED="$PSRAM" \
        -DUSB_HID_ENABLED=1 \
        -DDEBUG_LOGS_ENABLED=OFF \
        -DFIRMWARE_VERSION="v${VERSION_DOT}" \
        -DMOS2=ON \
        > /dev/null 2>&1

    # Build
    if make -j8 > /dev/null 2>&1; then
        # Copy UF2 to release directory with MOS2 extension
        # The build produces murmc64.m1p2.uf2 or murmc64.m2p2.uf2
        MOS2_BUILD_NAME="murmc64.${MOS2_EXT}"
        if [[ -f "${MOS2_BUILD_NAME}.uf2" ]]; then
            cp "${MOS2_BUILD_NAME}.uf2" "$RELEASE_DIR/$OUTPUT_NAME"
            echo -e "  ${GREEN}✓ Success${NC} → release/$OUTPUT_NAME"
        else
            echo -e "  ${RED}✗ ${MOS2_EXT} file not found${NC}"
        fi
    else
        echo -e "  ${RED}✗ Build failed${NC}"
    fi

    cd "$SCRIPT_DIR"
done

# Clean up build directory
rm -rf build

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${GREEN}Release build complete!${NC}"
echo ""
echo "Release files in: $RELEASE_DIR/"
echo ""
echo "UF2 files (for BOOTSEL flashing):"
ls -la "$RELEASE_DIR"/murmc64_*_${VERSION}.uf2 2>/dev/null | awk '{print "  " $9 " (" $5 " bytes)"}'
echo ""
echo "MOS2 files (for Murmulator OS):"
ls -la "$RELEASE_DIR"/murmc64_*_${VERSION}.m?p2 2>/dev/null | awk '{print "  " $9 " (" $5 " bytes)"}'
echo ""
echo -e "Version: ${CYAN}${VERSION_DOT}${NC}"
