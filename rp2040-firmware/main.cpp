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
    // Initialize WS2812 LED (Waveshare RP2040 Zero)
    WS2812Led::init();
    WS2812Led::set_blue();  // Show blue on startup

    // Configure TXS0108E level shifter - set OE pin (GPIO 8) high to enable level shifting
    gpio_init(8);
    gpio_set_dir(8, GPIO_OUT);
    gpio_put(8, 1);  // OE high = enabled, allows signal passthrough to DRO

    // Initialize the USB device
    if (!USBDevice::instance().init()) {
        // USB init failed - show red LED
        WS2812Led::set_red();
        while(1) tight_loop_contents();
    }

    // Initialize position system (which also initializes encoders)
    Position& pos = Position::instance();

    // Set scale factors for each encoder
    // These convert encoder counts to meaningful units (e.g., mm, inches, degrees)
    // Example: if encoder has 1000 counts per mm, scale = 1.0/1000.0 = 0.001
    pos.set_scale(0, 0.001);  // X axis: 0.001 mm per count
    pos.set_scale(1, 0.001);  // Y axis: 0.001 mm per count
    pos.set_scale(2, 0.001);  // Z axis: 0.001 mm per count
    pos.set_scale(3, 0.1);    // A axis: 0.1 degrees per count
    
    // Enable test mode for development and testing
    pos.enable_test_mode(true);
    pos.set_test_pattern(0);  // 0=SINE_WAVE, 1=CIRCULAR, 2=LINEAR_RAMP, 3=RANDOM_WALK

    // Show green to indicate ready
    WS2812Led::set_green();
    sleep_ms(1000);
    WS2812Led::set_off();

    while (1) {
        USBDevice::instance().task();
        // Periodically check for encoder overflows
        QuadratureEncoder::instance().update_overflow_check();
    }
}
