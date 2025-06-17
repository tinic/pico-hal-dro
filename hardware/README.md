# Hardware Setup - RP2040 Quadrature Encoder Interface

This document describes the hardware setup for the RP2040-based USB quadrature encoder interface for LinuxCNC DRO (Digital Readout) systems.

## Overview

The system reads up to 4 quadrature encoders and provides position data over USB to LinuxCNC. It uses PIO state machines for high-speed, accurate encoder counting with 64-bit signed position counters.

## Required Components

### Core Components
- **Waveshare RP2040 Zero** - Compact RP2040 board with built-in WS2812 RGB LED
- **TXS0108E** - 8-channel bidirectional level shifter breakout board
- **10-Pin 2.54mm Pitch PCB Mount Screw Terminal Block** - For encoder connections
- **Glass scales** - Linear encoders from existing DRO system
- **TOAUTO Digital Readout (DRO)** - Existing DRO with 9-pin connector
- **Jienk DB9 Male to Female Terminal Breakout Board** - For signal interception

### Connection Method
This setup intercepts the signals between existing glass scales and the TOAUTO DRO unit. The DB9 breakout boards allow you to tap into the encoder signals without cutting cables, maintaining the original DRO functionality while adding LinuxCNC capability. The TXS0108E level shifter enables signal passthrough to the DRO while providing safe 3.3V levels for the RP2040.

## Pin Assignments

### Quadrature Encoder Connections
The system uses GPIO pins 0-7 for encoder inputs:

| Encoder | Axis | GPIO A | GPIO B | Description |
|---------|------|--------|--------|-------------|
| 0       | X    | 0      | 1      | X-axis linear scale |
| 1       | Y    | 2      | 3      | Y-axis linear scale |
| 2       | Z    | 4      | 5      | Z-axis linear scale |
| 3       | A    | 6      | 7      | A-axis rotary encoder |

### Control Pins
| Function | GPIO | Description |
|----------|------|-------------|
| Level Shifter OE | 8 | Output Enable for TXS0108E (active high) |
| Status LED | Board-specific | WS2812 RGB LED (Waveshare RP2040 Zero) |

## Wiring Diagram

```
                    ┌─────────────────┐
                    │   RP2040 Board  │
                    │                 │
    Encoder 0 A ────┤ GPIO 0          │
    Encoder 0 B ────┤ GPIO 1          │
    Encoder 1 A ────┤ GPIO 2          │
    Encoder 1 B ────┤ GPIO 3          │
    Encoder 2 A ────┤ GPIO 4          │
    Encoder 2 B ────┤ GPIO 5          │
    Encoder 3 A ────┤ GPIO 6          │
    Encoder 3 B ────┤ GPIO 7          │
                    │                 │
    Level Shift OE ─┤ GPIO 8          │
                    │                 │
                    │            USB  ├──── To LinuxCNC PC
                    │                 │
                    └─────────────────┘
```

## Level Shifter Setup (TXS0108E)

The TXS0108E is used to interface between the RP2040 (3.3V) and encoder signals (typically 5V):

### TXS0108E Connections
```
Waveshare RP2040 Zero       TXS0108E          Terminal Block (10-pin)
─────────────────────       ────────          ──────────────────────
Pin 1  (GPIO 0) ─────────── A1    B1 ─────────── Pin 1 (Encoder 0 A)
Pin 2  (GPIO 1) ─────────── A2    B2 ─────────── Pin 2 (Encoder 0 B)
Pin 3  (GPIO 2) ─────────── A3    B3 ─────────── Pin 3 (Encoder 1 A)
Pin 4  (GPIO 3) ─────────── A4    B4 ─────────── Pin 4 (Encoder 1 B)
Pin 5  (GPIO 4) ─────────── A5    B5 ─────────── Pin 5 (Encoder 2 A)
Pin 6  (GPIO 5) ─────────── A6    B6 ─────────── Pin 6 (Encoder 2 B)
Pin 7  (GPIO 6) ─────────── A7    B7 ─────────── Pin 7 (Encoder 3 A)
Pin 8  (GPIO 7) ─────────── A8    B8 ─────────── Pin 8 (Encoder 3 B)
Pin 9  (GPIO 8) ─────────── OE              
Pin 10 (3V3)    ─────────── VCCA             
Pin 13 (GND)    ─────────── GND   GND ─────────── Pin 9  (Common GND)
                            VCCB ─────────── Pin 10 (+5V Supply)
```

**Note**: The A-side of the TXS0108E is directly soldered to pins 1-9 of the Waveshare RP2040 Zero.

### Level Shifter Notes
- OE (Output Enable) pin is set HIGH to enable bidirectional level shifting
- This allows glass scale signals to pass through to the TOAUTO DRO while providing 3.3V levels to the RP2040
- The RP2040 GPIO pins are configured as inputs only, preventing any interference with the encoder signals
- The TXS0108E provides proper signal isolation and level conversion between 5V and 3.3V systems

## Encoder Specifications

### Supported Encoder Types
- **Glass scales** (linear encoders)
- **Rotary encoders** (optical, magnetic)
- **Incremental encoders** with A/B quadrature outputs

### Signal Requirements
- **Voltage levels**: Typically 5V logic (level-shifted to 3.3V)
- **Signal type**: Differential or single-ended quadrature
- **Frequency**: Up to several MHz (limited by PIO state machine speed)

## TOAUTO DRO Integration

### DB9 Connector Pinout (Per Axis)
The TOAUTO Digital Readout has one 9-pin DB9 connector per axis. Each connector uses the same pinout:

| Pin | Signal | Description |
|-----|--------|-------------|
| 1   | +5V    | Power supply |
| 2   | 0V     | Ground |
| 3   | A      | Quadrature A channel (TTL) |
| 4   | B      | Quadrature B channel (TTL) |
| 5   | NC     | Not connected |
| 6   | NC     | Not connected |
| 7   | NC     | Not connected |
| 8   | NC     | Not connected |
| 9   | NC     | Not connected |

**Signal Type**: TTL square wave signals (0V/5V logic levels)

### Signal Interception Strategy
For each axis you want to monitor:
1. Use one Jienk DB9 Male to Female Terminal Breakout Board per axis
2. Connect glass scale to the **Female DB9** connector
3. Connect TOAUTO DRO to the **Male DB9** connector  
4. Tap into pins 3 and 4 (A and B channels) for encoder signals
5. Use pin 1 (+5V) and pin 2 (GND) for power connections

### DB9 to Terminal Block Wiring
From each DB9 breakout board terminal block:
- **Pin 1 (+5V)** → Terminal block pin 10 (shared +5V supply)
- **Pin 2 (GND)** → Terminal block pin 9 (shared ground)
- **Pin 3 (A signal)** → Terminal block pins 1,3,5,7 (for X,Y,Z,A respectively)
- **Pin 4 (B signal)** → Terminal block pins 2,4,6,8 (for X,Y,Z,A respectively)

### Multiple Axis Setup
- **X-axis**: DB9 pins 3,4 → Terminal pins 1,2 → GPIO 0,1
- **Y-axis**: DB9 pins 3,4 → Terminal pins 3,4 → GPIO 2,3
- **Z-axis**: DB9 pins 3,4 → Terminal pins 5,6 → GPIO 4,5
- **A-axis**: DB9 pins 3,4 → Terminal pins 7,8 → GPIO 6,7

## Assembly Instructions

### 1. Prepare the RP2040 Board
- Flash the firmware (see rp2040-firmware/README.md)
- Test basic functionality with USB connection

### 2. Wire the Level Shifter
- Connect power supplies (3.3V to VCCA, 5V to VCCB)
- Connect grounds together
- Wire GPIO pins 0-7 to A1-A8 on TXS0108E
- Connect GPIO 8 to OE pin (will be set HIGH to enable level shifting)

### 3. Connect Encoders
- Wire encoder A/B signals to B1-B8 on TXS0108E
- Connect encoder power (typically +5V) to VCCB supply
- Ensure all grounds are connected together

### 4. Test the System
- Use the Python test script to verify encoder readings
- Check that position values change when encoders are moved
- Verify USB communication is working

## Troubleshooting

### No Encoder Readings
- Check power supply connections (3.3V and 5V)
- Verify level shifter OE pin is HIGH (enabled) during operation
- Test encoder signals with oscilloscope or multimeter
- Check GPIO pin assignments match firmware
- Ensure DB9 breakout boards are properly connected between glass scales and DRO

### Incorrect Count Direction
- Swap A and B signals for the affected encoder
- Or modify scale factor to negative value in firmware

### Noise or Erratic Readings
- Add pull-up resistors (1kΩ-10kΩ) on encoder signal lines
- Use shielded cables for encoder connections
- Check for proper grounding
- Verify power supply is clean and stable

### USB Connection Issues
- Check USB cable and connections
- Verify device appears with VID:PID 2e8a:c0de
- Try different USB port or cable

### DRO Not Working
- Verify TXS0108E OE pin is HIGH to allow signal passthrough
- Check all DB9 connections are secure
- Test that glass scale signals reach the TOAUTO DRO (pins 3,4 on each DB9)

## Configuration

### Scale Factors
Configure in `main.cpp` based on your encoder specifications:
```cpp
pos.set_scale(0, 0.001);  // X: 0.001 mm/count (1000 counts/mm)
pos.set_scale(1, 0.001);  // Y: 0.001 mm/count 
pos.set_scale(2, 0.001);  // Z: 0.001 mm/count
pos.set_scale(3, 0.1);    // A: 0.1 degrees/count (10 counts/degree)
```

### Common Scale Factor Examples
- **Glass scales**: 1μm = 0.001 mm/count (typical)
- **Rotary encoders**: Based on pulses per revolution
  - 1000 PPR → 0.36°/count (360°/1000)
  - 2500 PPR → 0.144°/count (360°/2500)

## Safety Notes

- Always power off the system before making wiring changes
- Use appropriate ESD protection when handling components
- Verify voltage levels before connecting encoders
- Double-check wiring before applying power
- Use proper enclosures in industrial environments