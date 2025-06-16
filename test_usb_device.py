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
    try:
        # Set the active configuration
        dev.set_configuration()
        
        # Claim the vendor interface
        cfg = dev.get_active_configuration()
        intf = cfg[(0, 0)]
        
        # Check if kernel driver is active and detach if needed
        try:
            if dev.is_kernel_driver_active(intf.bInterfaceNumber):
                dev.detach_kernel_driver(intf.bInterfaceNumber)
        except usb.core.USBError:
            # On macOS, this might not be needed or supported
            pass
        
        # Claim the interface
        usb.util.claim_interface(dev, intf.bInterfaceNumber)
        
        return intf
    except usb.core.USBError as e:
        print(f"USB setup error: {e}")
        print("Try running with sudo or check USB permissions")
        raise

def get_position_once(dev):
    """Get position data once"""
    try:
        # Send request
        result = dev.write(EP_OUT, [VENDOR_REQUEST_GET_POSITION], timeout=1000)
        if result != 1:
            return None
        
        # Wait for processing
        time.sleep(0.05)
        
        # Read response
        data = dev.read(EP_IN, 64, timeout=1000)
        
        # Parse the data (4 doubles = 32 bytes)
        if len(data) >= 32:
            positions = struct.unpack('<4d', bytes(data[:32]))
            return positions
        elif len(data) > 0:
            print(f"Warning: Received {len(data)} bytes, expected 32")
            
    except usb.core.USBTimeoutError:
        pass  # Timeout is normal, just return None
    except Exception as e:
        print(f"USB error: {e}")
        
    return None

def poll_positions(dev, duration=5, frequency=2):
    """Poll position data at specified frequency"""
    print(f"Polling positions for {duration} seconds at {frequency} Hz...")
    
    start_time = time.time()
    count = 0
    successful_reads = 0
    interval = 1.0 / frequency
    
    while time.time() - start_time < duration:
        positions = get_position_once(dev)
        count += 1
        
        if positions:
            successful_reads += 1
            print(f"[{successful_reads:4d}] X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}")
        else:
            print(f"[{count:4d}] No response")
        
        time.sleep(interval)
    
    success_rate = (successful_reads / count) * 100 if count > 0 else 0
    print(f"Success rate: {successful_reads}/{count} ({success_rate:.1f}%)")

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
        try:
            dev.write(EP_OUT, [VENDOR_REQUEST_ENABLE_TEST_MODE], timeout=1000)
            time.sleep(0.2)
            
            # Set pattern
            dev.write(EP_OUT, [VENDOR_REQUEST_SET_TEST_PATTERN, pattern_id], timeout=1000)
            time.sleep(0.2)
            
            # Read several positions to show the pattern
            successful_reads = 0
            for i in range(8):
                positions = get_position_once(dev)
                if positions:
                    successful_reads += 1
                    print(f"[{i+1:2d}] X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}")
                else:
                    print(f"[{i+1:2d}] No response")
                time.sleep(0.3)
            
            print(f"Pattern success rate: {successful_reads}/8")
            
        except Exception as e:
            print(f"Error testing {pattern_name}: {e}")
    
    # Disable test mode
    print("\nDisabling test mode...")
    try:
        dev.write(EP_OUT, [VENDOR_REQUEST_DISABLE_TEST_MODE], timeout=1000)
        time.sleep(0.2)
        
        # Verify test mode is off (should return to encoder data or zeros)
        positions = get_position_once(dev)
        if positions:
            print(f"Final positions: X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}")
        else:
            print("No response after disabling test mode")
    except Exception as e:
        print(f"Error disabling test mode: {e}")

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
            print(f"Positions: X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}")
        else:
            print("No response received")
        
        # Test position polling at a reasonable rate
        print("\nTesting position polling:")
        poll_positions(dev, duration=10, frequency=2)  # 2 Hz for 10 seconds
        
        # Test all test mode patterns
        test_mode_demo(dev)
        
    except usb.core.USBError as e:
        print(f"USB Error: {e}")
        print("\nTroubleshooting tips:")
        print("1. Try running with sudo: sudo python3 test_usb_device.py")
        print("2. Make sure the Pico is running the correct firmware")
        print("3. Try unplugging and reconnecting the device")
        print("4. Check if another process is using the device")
        sys.exit(1)
    except ValueError as e:
        print(f"Device Error: {e}")
        print("\nTroubleshooting tips:")
        print("1. Make sure the Pico is connected via USB")
        print("2. Verify the firmware is flashed correctly")
        print("3. Check the USB cable connection")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()