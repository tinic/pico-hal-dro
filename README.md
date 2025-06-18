# RP2040 LinuxCNC HAL Quadrature Encoder Interface

A USB quadrature encoder interface that connects DRO scales to LinuxCNC using an RP2040 microcontroller.

## Overview

This project allows you to connect existing DRO (Digital Read Out) scales to LinuxCNC through a simple USB connection. It uses readily available components:

**Waveshare RP2040-Zero:**

[<img src="./hardware/rp2040-zero.jpg" width="200px"/>](./hardware/rp2040-zero.jpg)

**TXS0108E 8 Channel Logic Level Converter:**

[<img src="./hardware/TXS0108E.jpg" width="200px"/>](./hardware/TXS0108E.jpg)

## Features

- Supports 4 quadrature encoders (X, Y, Z, A axes)
- High-speed PIO-based encoder counting
- 32-bit position counters
- USB interface with LinuxCNC HAL component
- Configurable scale factors
- Test mode for development and debugging

## Quick Start

See the individual directories for detailed instructions:
- [`rp2040-firmware/`](rp2040-firmware/) - RP2040 firmware
- [`linuxcnc-hal/`](linuxcnc-hal/) - LinuxCNC HAL component
- [`hardware/`](hardware/) - Hardware setup and wiring 
