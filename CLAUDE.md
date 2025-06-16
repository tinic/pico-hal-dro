# RP2040 LinuxCNC HAL USB Device

This project implements a USB device on RP2040-based boards that interfaces with LinuxCNC through a HAL (Hardware Abstraction Layer) driver.

## Project Structure

- `rp2040-firmware/` - RP2040 firmware
  - `main.cpp` - Main firmware entry point
  - `position.cpp/h` - Position tracking implementation
  - `usb_device.cpp/h` - USB device implementation
  - `tusb_config.h` - TinyUSB configuration
  - `CMakeLists.txt` - CMake build configuration
  - `pico-sdk/` - Pico SDK submodule
- `linuxcnc-hal/` - LinuxCNC HAL component
  - `rp2040_dro.comp` - HAL component source
  - `README.md` - Installation and usage instructions
  - `rp2040-dro-example.hal` - Example HAL configuration
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

### USB Device Testing
```bash
# Install pyusb for testing
pip install pyusb

# Run the test script (requires sudo on Linux)
python3 test_usb_device.py
```

The test script will:
1. Find the USB device (VID: 0x2E8A, PID: 0x10DF)
2. Request a single position reading
3. Stream position data for 5 seconds

## Deployment

### Flashing the RP2040
1. Hold the BOOTSEL button while connecting the RP2040 board to USB
2. The RP2040 will appear as a mass storage device
3. Copy the generated `build/pico-hal-dro.uf2` file to the device
4. The RP2040 will automatically reboot and run the firmware

### LinuxCNC HAL Component
See `linuxcnc-hal/README.md` for detailed installation and usage instructions.

Quick start:
```bash
cd linuxcnc-hal
sudo halcompile --install rp2040_dro.comp
```

## USB Protocol

The device implements a vendor-specific USB interface with the following commands:

- **0x01** - Get Position: Returns 32 bytes (4 doubles) with current position values
- **0x02** - Start Stream: Begins continuous streaming of position data at 100Hz
- **0x03** - Stop Stream: Stops the continuous stream
- **0x04** - Enable Test Mode: Enables test mode for simulated position data
- **0x05** - Disable Test Mode: Disables test mode and returns to encoder data
- **0x06** - Set Test Pattern: Sets the test pattern (requires 1 byte parameter)

Position data format: 4 double-precision floating point values (little-endian)

## Test Mode

The firmware includes a test mode feature that simulates realistic DRO/glass gauge behavior for development and testing purposes.

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

Or via USB commands in the test script:
```bash
python3 test_usb_device.py  # Now includes test mode demonstration
```