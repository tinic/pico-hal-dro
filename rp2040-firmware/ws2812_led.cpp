#include "ws2812_led.h"
#include "ws2812.pio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

static PIO pio = pio1;  // Use pio1 to avoid conflict with quadrature encoder
static uint sm = 0;
static bool initialized = false;

void WS2812Led::init() {
    if (initialized) return;
    
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, false);
    initialized = true;
}

void WS2812Led::set_color(uint8_t red, uint8_t green, uint8_t blue) {
    // WS2812 expects GRB format
    uint32_t pixel_grb = ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
    put_pixel(pixel_grb);
}

void WS2812Led::set_red() {
    set_color(255, 0, 0);
}

void WS2812Led::set_green() {
    set_color(0, 255, 0);
}

void WS2812Led::set_blue() {
    set_color(0, 0, 255);
}

void WS2812Led::set_off() {
    set_color(0, 0, 0);
}

void WS2812Led::put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}