# RP2040 LinuxCNC HAL Quadrature Encoder Interface

This project implements a USB quadrature encoder interface on RP2040-based boards that connects with LinuxCNC through a HAL (Hardware Abstraction Layer) component.

## Project Structure

- `rp2040-firmware/` - RP2040 firmware
  - `main.cpp` - Main firmware entry point
  - `position.cpp/h` - Position tracking implementation
  - `usb_device.cpp/h` - USB device implementation
  - `tusb_config.h` - TinyUSB configuration
  - `CMakeLists.txt` - CMake build configuration
  - `pico-sdk/` - Pico SDK submodule
- `linuxcnc-hal/` - LinuxCNC HAL component
  - `rp2040_encoder.comp` - HAL component source
  - `README.md` - Installation and usage instructions
  - `rp2040-encoder-example.hal` - Example HAL configuration
- `test_usb_device.py` - Python test script for USB communication

## Build Commands

### Firmware Build
```bash
cd rp2040-firmware
mkdir -p build
cd build
cmake ..
make
```

## Development Notes

- The project uses the RP2040 Pico SDK as a git submodule
- Firmware implements USB device functionality for LinuxCNC communication
- Position tracking is handled through dedicated position modules
- Quadrature encoders are read using PIO state machines for high-speed, accurate counting
- 64-bit signed counters with automatic overflow detection and handling
- Overflow detection based on large magnitude changes with sign flip detection
- Modern C++23 features including std::expected for error handling, constexpr, and noexcept
- Simple, clear interfaces focused on the specific use case

## Hardware Configuration

### Quadrature Encoder Connections
The system supports 4 quadrature encoders connected to GPIO pins 0-7:
- Encoder 0 (X-axis): GPIO 0 (A), GPIO 1 (B)
- Encoder 1 (Y-axis): GPIO 2 (A), GPIO 3 (B)
- Encoder 2 (Z-axis): GPIO 4 (A), GPIO 5 (B)
- Encoder 3 (A-axis): GPIO 6 (A), GPIO 7 (B)

### Encoder Scaling
Scale factors convert encoder counts to meaningful units. Configure in main.cpp:
- Linear axes (X,Y,Z): Default 0.001 mm/count (1000 counts/mm)
- Rotary axis (A): Default 0.1 degrees/count (10 counts/degree)

### Overflow Handling
- PIO hardware counters are 32-bit signed integers
- Software maintains 64-bit signed counters for extended range
- Automatic overflow detection every 1ms using sign-flip detection
- Handles both positive and negative overflows seamlessly
- No count loss during overflow events

## Testing

### USB Encoder Interface Testing
```bash
# Install pyusb for testing
pip install pyusb

# Run the test script (requires sudo on Linux)
python3 test_usb_device.py
```

The test script will:
1. Find the USB device (VID: 0x2E8A, PID: 0xC0DE)
2. Request a single position reading
3. Stream position data for 5 seconds

The device identifies itself with a descriptive serial number in the format:
`{N}ENC-{git_hash}-{build_date}` (e.g., "4ENC-2d49ae7-2025-06-17")

## Deployment

### Flashing the RP2040
1. Hold the BOOTSEL button while connecting the RP2040 board to USB
2. The RP2040 will appear as a mass storage device
3. Copy the generated `build/rp2040-hal-encoder.uf2` file to the device
4. The RP2040 will automatically reboot and run the firmware

### LinuxCNC HAL Component
See `linuxcnc-hal/README.md` for detailed installation and usage instructions.

Quick start:
```bash
cd linuxcnc-hal
sudo halcompile --install rp2040_encoder.comp
```

## USB Protocol

The device implements a vendor-specific USB interface with the following commands:

- **0x01** - Get Position: Returns 32 bytes (4 doubles) with current position values
- **0x02** - Set Test Mode: Sets test mode (requires 1 byte parameter)
  - 0 = Disable test mode (use real encoder data)
  - 1 = Sine wave pattern
  - 2 = Circular motion pattern
  - 3 = Linear ramp pattern
  - 4 = Random walk pattern
- **0x03** - Set Scale: Sets scale factor for an encoder (requires 1 byte encoder index + 8 bytes double scale factor)
- **0x04** - Get Scale: Returns 32 bytes (4 doubles) with scale factors for all encoders
- **0x05** - Reset Encoder: Resets encoder position to zero (requires 1 byte encoder index)

Position data format: 4 double-precision floating point values (little-endian)

## Test Mode

The firmware includes a test mode feature that simulates realistic quadrature encoder behavior for development and testing purposes.

### Test Patterns

- **0 - Sine Wave**: Sinusoidal motion on all axes with different frequencies and amplitudes
- **1 - Circular**: XY circular motion with Z oscillation and A rotation
- **2 - Linear Ramp**: Constant velocity motion on all axes
- **3 - Random Walk**: Random position changes simulating measurement noise

### Usage

Test mode can be controlled via USB commands or enabled in firmware:

```cpp
// Enable in firmware (main.cpp)
pos.enable_test_mode(true);
pos.set_test_pattern(0);  // 0=SINE_WAVE, 1=CIRCULAR, 2=LINEAR_RAMP, 3=RANDOM_WALK
```

Or via USB commands in the test script or HAL component:
```bash
python3 test_usb_device.py  # Now includes test mode demonstration
```