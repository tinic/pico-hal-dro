# LinuxCNC HAL Component for RP2040 HAL DRO

This directory contains the LinuxCNC HAL component for interfacing with the RP2040 USB DRO device.

## Requirements

- LinuxCNC 2.7 or later
- libusb-1.0 development files (`sudo apt-get install libusb-1.0-0-dev`)
- halcompile (included with LinuxCNC)

## Installation

1. Compile and install the component:
   ```bash
   sudo halcompile --install pico_dro.comp
   ```

2. Add udev rule for USB access (create `/etc/udev/rules.d/99-rp2040-dro.rules`):
   ```
   SUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="10df", MODE="0666"
   ```

3. Reload udev rules:
   ```bash
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```

## Usage

In your HAL configuration file:

```hal
# Load the component
loadusr -W pico_dro

# Connect position outputs to your DRO display
net x-pos pico_dro.0.position-0 => pyvcp.x-dro
net y-pos pico_dro.0.position-1 => pyvcp.y-dro
net z-pos pico_dro.0.position-2 => pyvcp.z-dro
net a-pos pico_dro.0.position-3 => pyvcp.a-dro

# Monitor connection status
net dro-connected pico_dro.0.connected => pyvcp.dro-connected-led
```

## HAL Pins

- `pico_dro.0.position-0` (float, out) - Position 0 value
- `pico_dro.0.position-1` (float, out) - Position 1 value
- `pico_dro.0.position-2` (float, out) - Position 2 value
- `pico_dro.0.position-3` (float, out) - Position 3 value
- `pico_dro.0.connected` (bit, out) - True when USB device is connected

## Troubleshooting

1. Check USB connection:
   ```bash
   lsusb | grep 2e8a:10df
   ```

2. Check HAL component is loaded:
   ```bash
   halcmd show comp pico_dro
   ```

3. Monitor HAL pins:
   ```bash
   halcmd show pin pico_dro
   ```

4. Check for USB permission issues:
   ```bash
   dmesg | tail
   ```