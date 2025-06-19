#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}RP2040 Encoder Firmware Build and Release Script${NC}"
echo "================================================"

# Check if we're in the root directory
if [ ! -f "README.md" ] || [ ! -d "rp2040-firmware" ]; then
    echo -e "${RED}Error: This script must be run from the project root directory${NC}"
    exit 1
fi

# Get version info
GIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_DATE=$(date +%Y-%m-%d)
VERSION_TAG="v${BUILD_DATE}-${GIT_HASH}"

echo -e "${YELLOW}Version: ${VERSION_TAG}${NC}"

# Build firmware
echo -e "\n${GREEN}Building firmware...${NC}"
cd rp2040-firmware

# Clean previous build
if [ -d "build" ]; then
    echo "Cleaning previous build..."
    rm -rf build
fi

mkdir -p build
cd build

# Configure CMake
echo "Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building firmware..."
make -j$(nproc)

# Check if build succeeded
if [ ! -f "rp2040-hal-encoder.uf2" ]; then
    echo -e "${RED}Error: Build failed - UF2 file not found${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Copy firmware to release directory
cd ../..
RELEASE_DIR="release"
mkdir -p ${RELEASE_DIR}

FIRMWARE_NAME="rp2040-encoder-${VERSION_TAG}.uf2"
cp rp2040-firmware/build/rp2040-hal-encoder.uf2 ${RELEASE_DIR}/${FIRMWARE_NAME}

echo -e "${GREEN}Firmware copied to: ${RELEASE_DIR}/${FIRMWARE_NAME}${NC}"

# Create release notes
RELEASE_NOTES="${RELEASE_DIR}/RELEASE_NOTES.md"
cat > ${RELEASE_NOTES} << EOF
# RP2040 Encoder Firmware ${VERSION_TAG}

## Release Information
- **Version**: ${VERSION_TAG}
- **Build Date**: ${BUILD_DATE}
- **Git Hash**: ${GIT_HASH}

## Features
- USB quadrature encoder interface for LinuxCNC
- Support for 4 encoders (X, Y, Z, A axes)
- TTL level A/B quadrature signal input
- PIO-based hardware counting for high accuracy
- Test mode with multiple patterns for development

## Installation
1. Hold the BOOTSEL button while connecting the RP2040 board to USB
2. Copy the \`${FIRMWARE_NAME}\` file to the RP2040 mass storage device
3. The board will automatically reboot and run the firmware

## Compatibility
- Compatible with RP2040-based boards (Raspberry Pi Pico, etc.)
- Works with 5V TTL quadrature encoder scales
- LinuxCNC HAL component available in \`linuxcnc-hal/\` directory
EOF

echo -e "${GREEN}Release notes created${NC}"

# GitHub release (if gh CLI is available and we're in a git repo)
if command -v gh &> /dev/null && git rev-parse --git-dir > /dev/null 2>&1; then
    echo -e "\n${YELLOW}Creating GitHub release...${NC}"
    
    # Check if tag already exists
    if git rev-parse ${VERSION_TAG} >/dev/null 2>&1; then
        echo -e "${YELLOW}Warning: Tag ${VERSION_TAG} already exists${NC}"
        read -p "Do you want to delete the existing tag and recreate it? (y/N) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            git tag -d ${VERSION_TAG}
            git push origin :refs/tags/${VERSION_TAG} 2>/dev/null || true
        else
            echo -e "${YELLOW}Skipping GitHub release${NC}"
            echo -e "\n${GREEN}Build complete! Firmware available in ${RELEASE_DIR}/${NC}"
            exit 0
        fi
    fi
    
    # Create and push tag
    git tag -a ${VERSION_TAG} -m "Release ${VERSION_TAG}"
    git push origin ${VERSION_TAG}
    
    # Create GitHub release
    gh release create ${VERSION_TAG} \
        --title "RP2040 Encoder Firmware ${VERSION_TAG}" \
        --notes-file ${RELEASE_NOTES} \
        ${RELEASE_DIR}/${FIRMWARE_NAME}
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}GitHub release created successfully!${NC}"
        echo -e "Release URL: https://github.com/$(gh repo view --json nameWithOwner -q .nameWithOwner)/releases/tag/${VERSION_TAG}"
    else
        echo -e "${RED}Failed to create GitHub release${NC}"
    fi
else
    echo -e "${YELLOW}GitHub CLI (gh) not found or not in a git repository${NC}"
    echo "To publish to GitHub, install gh and run:"
    echo "  gh release create ${VERSION_TAG} --title \"RP2040 Encoder Firmware ${VERSION_TAG}\" --notes-file ${RELEASE_NOTES} ${RELEASE_DIR}/${FIRMWARE_NAME}"
fi

echo -e "\n${GREEN}Build complete!${NC}"
echo -e "Firmware: ${RELEASE_DIR}/${FIRMWARE_NAME}"
echo -e "Release notes: ${RELEASE_NOTES}"