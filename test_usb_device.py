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
VENDOR_REQUEST_ENABLE_TEST_MODE = 0x02
VENDOR_REQUEST_DISABLE_TEST_MODE = 0x03
VENDOR_REQUEST_SET_TEST_PATTERN = 0x04

# Test patterns
TEST_PATTERN_SINE_WAVE = 0
TEST_PATTERN_CIRCULAR = 1
TEST_PATTERN_LINEAR_RAMP = 2
TEST_PATTERN_RANDOM_WALK = 3

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

def poll_positions(dev, duration=5, frequency=10):
    """Poll position data at specified frequency"""
    print(f"Polling positions for {duration} seconds at {frequency} Hz...")
    
    start_time = time.time()
    count = 0
    interval = 1.0 / frequency
    
    while time.time() - start_time < duration:
        try:
            # Request position
            dev.write(EP_OUT, [VENDOR_REQUEST_GET_POSITION])
            
            # Read response
            data = dev.read(EP_IN, 64, timeout=100)
            if len(data) >= 32:
                positions = struct.unpack('<4d', bytes(data[:32]))
                count += 1
                print(f"[{count:4d}] Positions: {positions}")
            
            time.sleep(interval)
        except usb.core.USBTimeoutError:
            print("Timeout reading position")
        except Exception as e:
            print(f"Error: {e}")
    
    print(f"Received {count} position updates")

def test_mode_demo(dev):
    """Demonstrate test mode functionality"""
    print("\nTesting test mode functionality:")
    
    patterns = [
        (TEST_PATTERN_SINE_WAVE, "Sine Wave"),
        (TEST_PATTERN_CIRCULAR, "Circular Motion"),
        (TEST_PATTERN_LINEAR_RAMP, "Linear Ramp"),
        (TEST_PATTERN_RANDOM_WALK, "Random Walk")
    ]
    
    for pattern_id, pattern_name in patterns:
        print(f"\n--- Testing {pattern_name} Pattern ---")
        
        # Enable test mode
        dev.write(EP_OUT, [VENDOR_REQUEST_ENABLE_TEST_MODE])
        time.sleep(0.1)
        
        # Set pattern
        dev.write(EP_OUT, [VENDOR_REQUEST_SET_TEST_PATTERN, pattern_id])
        time.sleep(0.1)
        
        # Read a few positions
        for i in range(10):
            positions = get_position_once(dev)
            if positions:
                print(f"[{i+1:2d}] X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}")
            time.sleep(0.2)
    
    # Disable test mode
    print("\nDisabling test mode...")
    dev.write(EP_OUT, [VENDOR_REQUEST_DISABLE_TEST_MODE])
    time.sleep(0.1)
    
    # Verify test mode is off
    positions = get_position_once(dev)
    if positions:
        print(f"Final positions: X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}")

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
        
        # Test polling
        print("\nTesting position polling:")
        poll_positions(dev, 5, 10)
        
        # Test the new test mode functionality
        test_mode_demo(dev)
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()