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

int main() {
    // Initialize onboard LED (GPIO 25)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Initialize the USB device
    if (auto result = USBDevice::instance().init(); !result) {
        // USB init failed - this is a critical error for our DRO system
        // In a real system, we might blink an error LED or halt
        // For now, continue anyway as the encoders might still work locally
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

    while (1) {
        USBDevice::instance().task();
        // Periodically check for encoder overflows
        QuadratureEncoder::instance().update_overflow_check();
    }
}
