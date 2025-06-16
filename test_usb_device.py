#!/usr/bin/env python3
"""
Test script for the Pico HAL DRO USB device.
Requires pyusb: pip install pyusb
"""

import usb.core
import usb.util
import struct
import time
import sys

# USB device identifiers
VENDOR_ID = 0x2E8A  # Raspberry Pi
PRODUCT_ID = 0x10DF  # Our custom product ID

# Request codes
VENDOR_REQUEST_GET_POSITION = 0x01
VENDOR_REQUEST_START_STREAM = 0x02
VENDOR_REQUEST_STOP_STREAM = 0x03

# Endpoints
EP_IN = 0x81
EP_OUT = 0x01

def find_device():
    """Find the USB device"""
    dev = usb.core.find(idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
    if dev is None:
        raise ValueError("Device not found")
    return dev

def setup_device(dev):
    """Setup the USB device"""
    # Set the active configuration
    dev.set_configuration()
    
    # Claim the vendor interface
    cfg = dev.get_active_configuration()
    intf = cfg[(0, 0)]
    
    if dev.is_kernel_driver_active(intf.bInterfaceNumber):
        dev.detach_kernel_driver(intf.bInterfaceNumber)
    
    return intf

def get_position_once(dev):
    """Get position data once"""
    # Send request
    dev.write(EP_OUT, [VENDOR_REQUEST_GET_POSITION])
    
    # Read response
    data = dev.read(EP_IN, 64, timeout=1000)
    
    # Parse the data (4 doubles = 32 bytes)
    if len(data) >= 32:
        positions = struct.unpack('<4d', bytes(data[:32]))
        return positions
    return None

def stream_positions(dev, duration=5):
    """Stream position data continuously"""
    print(f"Starting position stream for {duration} seconds...")
    
    # Start streaming
    dev.write(EP_OUT, [VENDOR_REQUEST_START_STREAM])
    
    start_time = time.time()
    count = 0
    
    try:
        while time.time() - start_time < duration:
            try:
                data = dev.read(EP_IN, 64, timeout=100)
                if len(data) >= 32:
                    positions = struct.unpack('<4d', bytes(data[:32]))
                    count += 1
                    print(f"[{count:4d}] Positions: {positions}")
            except usb.core.USBTimeoutError:
                pass
    finally:
        # Stop streaming
        dev.write(EP_OUT, [VENDOR_REQUEST_STOP_STREAM])
        print(f"Received {count} position updates")

def main():
    try:
        # Find and setup device
        print("Looking for Pico HAL DRO device...")
        dev = find_device()
        print(f"Found device: {dev}")
        
        intf = setup_device(dev)
        print(f"Device configured, interface: {intf.bInterfaceNumber}")
        
        # Test single position read
        print("\nTesting single position read:")
        positions = get_position_once(dev)
        if positions:
            print(f"Positions: {positions}")
        
        # Test streaming
        print("\nTesting position streaming:")
        stream_positions(dev, 5)
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()