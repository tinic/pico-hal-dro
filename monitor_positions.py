#!/usr/bin/env python3
"""
Continuous position monitor for the RP2040 HAL DRO USB device.
Provides real-time position display with various display modes and logging options.
Requires pyusb: pip install pyusb
"""

import usb.core
import usb.util
import struct
import time
import sys
import argparse
import datetime
import csv
import os
from collections import deque
import signal

# USB device identifiers
VENDOR_ID = 0x2E8A  # Raspberry Pi Foundation (RP2040)
PRODUCT_ID = 0xC0DE  # Our custom product ID

# Request codes
VENDOR_REQUEST_GET_POSITION = 0x01
VENDOR_REQUEST_SET_TEST_MODE = 0x02
VENDOR_REQUEST_GET_SCALE = 0x04

# Sentinel values for data validation
POSITION_DATA_SENTINEL = 0x3F8A7C91
SCALE_DATA_SENTINEL = 0x7B2D4E8F

# Endpoints
EP_IN = 0x81
EP_OUT = 0x01

class PositionMonitor:
    def __init__(self, dev, args):
        self.dev = dev
        self.args = args
        self.running = True
        self.positions_history = deque(maxlen=100)  # Keep last 100 readings
        self.start_positions = None
        self.max_positions = None
        self.min_positions = None
        self.csv_writer = None
        self.csv_file = None
        self.read_count = 0
        self.success_count = 0
        self.start_time = time.time()
        
        # Setup signal handler for clean exit
        signal.signal(signal.SIGINT, self.signal_handler)
        
        # Setup CSV logging if requested
        if args.log:
            self.setup_csv_logging()
    
    def signal_handler(self, sig, frame):
        """Handle Ctrl+C gracefully"""
        self.running = False
        print("\n\nShutting down...")
    
    def setup_csv_logging(self):
        """Setup CSV file for logging positions"""
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"position_log_{timestamp}.csv"
        self.csv_file = open(filename, 'w', newline='')
        self.csv_writer = csv.writer(self.csv_file)
        self.csv_writer.writerow(['timestamp', 'x', 'y', 'z', 'a', 'elapsed_ms'])
        print(f"Logging positions to: {filename}")
    
    def get_position_fast(self):
        """Get position data with minimal delays"""
        try:
            # Send request with short timeout
            result = self.dev.write(EP_OUT, [VENDOR_REQUEST_GET_POSITION], timeout=10)
            if result != 1:
                return None
            
            # Read response with short timeout
            data = self.dev.read(EP_IN, 64, timeout=10)
            
            # Parse the data: [sentinel:4 bytes][positions:32 bytes] = 36 bytes total
            if len(data) >= 36:
                # Validate sentinel first
                sentinel = struct.unpack('<L', bytes(data[:4]))[0]
                if sentinel != POSITION_DATA_SENTINEL:
                    # Clear USB read queue to remove any stale data
                    try:
                        while True:
                            self.dev.read(EP_IN, 64, timeout=1)
                    except usb.core.USBTimeoutError:
                        pass  # Queue is now empty
                    return None
                
                # Extract position data after sentinel
                positions = struct.unpack('<4d', bytes(data[4:36]))
                return positions
                
        except usb.core.USBTimeoutError:
            pass  # Timeout is normal, just return None
        except Exception as e:
            if self.args.verbose:
                print(f"\nUSB error: {e}")
            
        return None
    
    def update_statistics(self, positions):
        """Update min/max statistics"""
        if self.start_positions is None:
            self.start_positions = positions
            self.max_positions = list(positions)
            self.min_positions = list(positions)
        else:
            for i in range(4):
                self.max_positions[i] = max(self.max_positions[i], positions[i])
                self.min_positions[i] = min(self.min_positions[i], positions[i])
    
    def format_position_line(self, positions, elapsed):
        """Format position data for display"""
        rate = self.success_count / elapsed if elapsed > 0 else 0
        success_pct = (self.success_count / self.read_count * 100) if self.read_count > 0 else 0
        
        if self.args.mode == 'simple':
            return f"X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}"
        
        elif self.args.mode == 'delta':
            if self.start_positions:
                deltas = [positions[i] - self.start_positions[i] for i in range(4)]
                return f"ΔX:{deltas[0]:8.3f} ΔY:{deltas[1]:8.3f} ΔZ:{deltas[2]:8.3f} ΔA:{deltas[3]:8.1f}"
            else:
                return f"X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}"
        
        elif self.args.mode == 'detailed':
            line = f"X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f} | "
            line += f"{rate:6.1f} Hz | {success_pct:5.1f}% | {elapsed:7.1f}s"
            return line
        
        elif self.args.mode == 'stats':
            if self.min_positions and self.max_positions:
                ranges = [self.max_positions[i] - self.min_positions[i] for i in range(4)]
                line = f"X:{positions[0]:8.3f} (R:{ranges[0]:6.3f}) "
                line += f"Y:{positions[1]:8.3f} (R:{ranges[1]:6.3f}) "
                line += f"Z:{positions[2]:8.3f} (R:{ranges[2]:6.3f}) "
                line += f"A:{positions[3]:8.1f} (R:{ranges[3]:6.1f})"
                return line
            else:
                return f"X:{positions[0]:8.3f} Y:{positions[1]:8.3f} Z:{positions[2]:8.3f} A:{positions[3]:8.1f}"
    
    def run(self):
        """Main monitoring loop"""
        print(f"Monitoring positions in '{self.args.mode}' mode")
        print(f"Update rate: {self.args.rate} Hz")
        if self.args.duration:
            print(f"Duration: {self.args.duration} seconds")
        else:
            print("Press Ctrl+C to stop")
        print("-" * 80)
        
        interval = 1.0 / self.args.rate if self.args.rate > 0 else 0
        last_update_time = time.time()
        next_read_time = time.time()
        
        while self.running:
            current_time = time.time()
            elapsed = current_time - self.start_time
            
            # Check duration limit
            if self.args.duration and elapsed >= self.args.duration:
                self.running = False
                break
            
            # Read position if it's time
            if current_time >= next_read_time:
                positions = self.get_position_fast()
                self.read_count += 1
                
                if positions:
                    self.success_count += 1
                    self.positions_history.append(positions)
                    self.update_statistics(positions)
                    
                    # Log to CSV if enabled
                    if self.csv_writer:
                        timestamp = datetime.datetime.now().isoformat()
                        elapsed_ms = (current_time - self.start_time) * 1000
                        self.csv_writer.writerow([timestamp, positions[0], positions[1], positions[2], positions[3], elapsed_ms])
                    
                    # Update display
                    if not self.args.quiet:
                        line = self.format_position_line(positions, elapsed)
                        if self.args.newline:
                            print(f"[{self.success_count:6d}] {line}")
                        else:
                            print(f"\r[{self.success_count:6d}] {line}", end="", flush=True)
                
                elif self.args.verbose:
                    if self.args.newline:
                        print(f"[{self.read_count:6d}] No response")
                    else:
                        print(f"\r[{self.read_count:6d}] No response" + " " * 50, end="", flush=True)
                
                # Schedule next read
                if interval > 0:
                    next_read_time += interval
                    # If we're behind schedule, catch up
                    if next_read_time < current_time:
                        next_read_time = current_time
            
            # Small sleep to prevent CPU spinning
            sleep_time = next_read_time - time.time()
            if sleep_time > 0.001:
                time.sleep(min(sleep_time, 0.001))
        
        # Print summary
        self.print_summary()
        
        # Close CSV file if open
        if self.csv_file:
            self.csv_file.close()
    
    def print_summary(self):
        """Print monitoring summary"""
        elapsed = time.time() - self.start_time
        if not self.args.newline:
            print()  # New line after continuous display
        
        print("\n" + "=" * 80)
        print("Monitoring Summary")
        print("=" * 80)
        print(f"Duration: {elapsed:.2f} seconds")
        print(f"Total reads: {self.read_count}")
        print(f"Successful reads: {self.success_count}")
        print(f"Success rate: {(self.success_count/self.read_count*100) if self.read_count > 0 else 0:.1f}%")
        print(f"Actual read rate: {self.read_count/elapsed if elapsed > 0 else 0:.1f} Hz")
        print(f"Successful read rate: {self.success_count/elapsed if elapsed > 0 else 0:.1f} Hz")
        
        if self.min_positions and self.max_positions and self.start_positions:
            print("\nPosition Statistics:")
            print(f"{'Axis':>4} {'Start':>10} {'Min':>10} {'Max':>10} {'Range':>10} {'Current':>10}")
            print("-" * 66)
            
            last_positions = self.positions_history[-1] if self.positions_history else self.start_positions
            axes = ['X', 'Y', 'Z', 'A']
            for i in range(4):
                print(f"{axes[i]:>4} {self.start_positions[i]:>10.3f} {self.min_positions[i]:>10.3f} "
                      f"{self.max_positions[i]:>10.3f} {self.max_positions[i]-self.min_positions[i]:>10.3f} "
                      f"{last_positions[i]:>10.3f}")

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

def main():
    parser = argparse.ArgumentParser(description='Continuous position monitor for RP2040 encoder interface')
    parser.add_argument('-r', '--rate', type=float, default=100.0,
                        help='Update rate in Hz (default: 100)')
    parser.add_argument('-d', '--duration', type=float,
                        help='Monitoring duration in seconds (default: continuous)')
    parser.add_argument('-m', '--mode', choices=['simple', 'delta', 'detailed', 'stats'],
                        default='simple',
                        help='Display mode (default: simple)')
    parser.add_argument('-l', '--log', action='store_true',
                        help='Log positions to CSV file')
    parser.add_argument('-n', '--newline', action='store_true',
                        help='Print each update on a new line')
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='Suppress position output (only show summary)')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Show verbose output including errors')
    
    args = parser.parse_args()
    
    try:
        # Find and setup device
        if not args.quiet:
            print("Looking for RP2040 HAL DRO device...")
        
        dev = find_device()
        
        # Get device serial number
        try:
            serial = usb.util.get_string(dev, dev.iSerialNumber)
            if not args.quiet:
                print(f"Found device: {dev}")
                print(f"Serial: {serial}")
        except:
            if not args.quiet:
                print(f"Found device: {dev}")
                print("Serial: Unable to read")
        
        intf = setup_device(dev)
        if not args.quiet:
            print(f"Device configured, interface: {intf.bInterfaceNumber}\n")
        
        # Create and run monitor
        monitor = PositionMonitor(dev, args)
        monitor.run()
        
    except usb.core.USBError as e:
        print(f"USB Error: {e}")
        print("\nTroubleshooting tips:")
        print("1. Try running with sudo: sudo python3 monitor_positions.py")
        print("2. Make sure the RP2040 is running the correct firmware")
        print("3. Try unplugging and reconnecting the device")
        print("4. Check if another process is using the device")
        sys.exit(1)
    except ValueError as e:
        print(f"Device Error: {e}")
        print("\nTroubleshooting tips:")
        print("1. Make sure the RP2040 board is connected via USB")
        print("2. Verify the firmware is flashed correctly")
        print("3. Check the USB cable connection")
        sys.exit(1)
    except KeyboardInterrupt:
        # Handled by signal handler
        pass
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()