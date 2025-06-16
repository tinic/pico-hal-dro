# Raspberry Pi Pico LinuxCNC HAL USB Device

This project implements a USB device on the Raspberry Pi Pico that interfaces with LinuxCNC through a HAL (Hardware Abstraction Layer) driver.

## Project Structure

- `pico-firmware/` - Raspberry Pi Pico firmware
  - `main.cpp` - Main firmware entry point
  - `position.cpp/h` - Position tracking implementation
  - `usb_device.cpp/h` - USB device implementation
  - `tusb_config.h` - TinyUSB configuration
  - `CMakeLists.txt` - CMake build configuration
  - `pico-sdk/` - Pico SDK submodule
- `linuxcnc-hal/` - LinuxCNC HAL component
  - `pico_dro.comp` - HAL component source
  - `README.md` - Installation and usage instructions
  - `pico-dro-example.hal` - Example HAL configuration
- `test_usb_device.py` - Python test script for USB communication

## Build Commands

### Firmware Build
```bash
cd pico-firmware
mkdir -p build
cd build
cmake ..
make
```

## Development Notes

- The project uses the Raspberry Pi Pico SDK as a git submodule
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

### Flashing the Pico
1. Hold the BOOTSEL button while connecting the Pico to USB
2. The Pico will appear as a mass storage device
3. Copy the generated `build/pico-hal-dro.uf2` file to the device
4. The Pico will automatically reboot and run the firmware

### LinuxCNC HAL Component
See `linuxcnc-hal/README.md` for detailed installation and usage instructions.

Quick start:
```bash
cd linuxcnc-hal
sudo halcompile --install pico_dro.comp
```

## USB Protocol

The device implements a vendor-specific USB interface with the following commands:

- **0x01** - Get Position: Returns 32 bytes (4 doubles) with current position values
- **0x02** - Start Stream: Begins continuous streaming of position data at 100Hz
- **0x03** - Stop Stream: Stops the continuous stream

Position data format: 4 double-precision floating point values (little-endian)