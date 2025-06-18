#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pll.h"
#include "hardware/sync.h"
#include "hardware/xosc.h"
#include "pico/stdlib.h"
#include "position.h"
#include "quadrature_encoder.h"
#include "usb_device.h"
#include "ws2812_led.h"

int main() {
    // Initialize WS2812 LED (auto-initializes on first access)
    WS2812Led::instance().set_blue();  // Show blue on startup

    // Configure TXS0108E level shifter - set OE pin (GPIO 8) high to enable level shifting
    gpio_init(8);
    gpio_set_dir(8, GPIO_OUT);
    gpio_put(8, 1);  // OE high = enabled, allows signal passthrough to DRO

    // Don't initialize GPIO pins 0-7 here - let PIO handle them
    // The quadrature_encoder_program_init() function will call pio_gpio_init()
    // which properly configures the pins for PIO use

    // Access USB device instance (auto-initializes)
    USBDevice::instance();

    // Initialize position system (which also auto-initializes encoders)
    Position& pos = Position::instance();

    // Set scale factors for each encoder
    // These convert encoder counts to meaningful units (e.g., mm, inches, degrees)
    // Example: if encoder has 1000 counts per mm, scale = 1.0/1000.0 = 0.001
    pos.set_scale(0, 0.001);  // X axis: 0.001 mm per count
    pos.set_scale(1, 0.001);  // Y axis: 0.001 mm per count
    pos.set_scale(2, 0.001);  // Z axis: 0.001 mm per count
    pos.set_scale(3, 0.1);    // A axis: 0.1 degrees per count
    
    // Test mode is disabled by default - controlled via USB commands
    pos.enable_test_mode(false);

    // Show green to indicate ready
    WS2812Led::instance().set_green();

    while (1) {
        USBDevice::instance().task();
        // Periodically drain encoder FIFOs
        QuadratureEncoder::instance().update_fifo_drain();
    }
}
