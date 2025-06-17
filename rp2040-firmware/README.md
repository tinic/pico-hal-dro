# RP2040 Quadrature Encoder Firmware

This directory contains the firmware for the RP2040-based USB quadrature encoder interface that connects with LinuxCNC through a HAL (Hardware Abstraction Layer) component.

## Features

- Supports 4 quadrature encoders on GPIO pins 0-7
- 64-bit signed position counters with automatic overflow handling
- High-speed PIO state machines for accurate encoder counting
- USB interface with 100Hz streaming capability
- Test mode with multiple simulation patterns
- Modern C++23 implementation with error handling

## Hardware Configuration

### Quadrature Encoder Connections
- Encoder 0 (X-axis): GPIO 0 (A), GPIO 1 (B)
- Encoder 1 (Y-axis): GPIO 2 (A), GPIO 3 (B)
- Encoder 2 (Z-axis): GPIO 4 (A), GPIO 5 (B)
- Encoder 3 (A-axis): GPIO 6 (A), GPIO 7 (B)

### Encoder Scaling
Scale factors are configured in `main.cpp`:
- Linear axes (X,Y,Z): Default 0.001 mm/count (1000 counts/mm)
- Rotary axis (A): Default 0.1 degrees/count (10 counts/degree)

## Requirements

- RP2040-based board (Raspberry Pi Pico, Pico W, etc.)
- CMake 3.13 or later
- GCC ARM cross-compiler
- Pico SDK (included as submodule)

## Building

1. Initialize the Pico SDK submodule:
   ```bash
   git submodule update --init --recursive
   ```

2. Create build directory and configure:
   ```bash
   cd rp2040-firmware
   mkdir -p build
   cd build
   cmake ..
   ```

3. Build the firmware:
   ```bash
   make
   ```

   This will generate `rp2040-hal-encoder.uf2` in the build directory.

## Flashing

### Method 1: BOOTSEL Mode (Recommended)
1. Hold the BOOTSEL button while connecting the RP2040 board to USB
2. The RP2040 will appear as a mass storage device (RPI-RP2)
3. Copy `build/rp2040-hal-encoder.uf2` to the device
4. The RP2040 will automatically reboot and run the firmware

### Method 2: Using picotool (if installed)
```bash
# Put device in BOOTSEL mode first
picotool load build/rp2040-hal-encoder.uf2
picotool reboot
```

## USB Protocol

The device implements a vendor-specific USB interface (VID: 0x2E8A, PID: 0xC0DE) with the following commands:

- **0x01** - Get Position: Returns 32 bytes (4 doubles) with current position values
- **0x02** - Start Stream: Begins continuous streaming at 100Hz
- **0x03** - Stop Stream: Stops the continuous stream
- **0x04** - Enable Test Mode: Enables test mode for simulated data
- **0x05** - Disable Test Mode: Returns to encoder data
- **0x06** - Set Test Pattern: Sets test pattern (0-3)

## Test Mode

The firmware includes test mode for development and testing:

### Test Patterns
- **0 - Sine Wave**: Sinusoidal motion with different frequencies per axis
- **1 - Circular**: XY circular motion with Z oscillation and A rotation
- **2 - Linear Ramp**: Constant velocity motion on all axes
- **3 - Random Walk**: Random position changes simulating noise

### Enabling Test Mode
```cpp
// In main.cpp
pos.enable_test_mode(true);
pos.set_test_pattern(0);  // Set desired pattern
```

## Testing

Use the Python test script in the project root:
```bash
# Install dependencies
pip install pyusb

# Run test (requires sudo on Linux)
python3 test_usb_device.py
```

## Device Identification

The device serial number format: `{N}ENC-{git_hash}-{build_date}`
- N: Number of encoders (4)
- git_hash: Current git commit hash
- build_date: Firmware build date

Example: `4ENC-2d49ae7-2025-06-17`

## Troubleshooting

1. **Build fails**: Ensure Pico SDK submodule is initialized and CMake version ≥ 3.13

2. **Device not recognized**: Check USB connection and verify device appears with correct VID:PID (2e8a:c0de)

3. **Position data incorrect**: Verify encoder wiring matches GPIO pin assignments

4. **Flashing fails**: Ensure board is in BOOTSEL mode (hold button during USB connection)

For LinuxCNC HAL component usage, see `../linuxcnc-hal/README.md`.