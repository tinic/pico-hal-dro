#ifndef WS2812_LED_H_
#define WS2812_LED_H_

#include <cstdint>

class WS2812Led {
public:
    static constexpr uint8_t LED_PIN = 16;  // Waveshare RP2040 Zero WS2812 pin
    
    static void init();
    static void set_color(uint8_t red, uint8_t green, uint8_t blue);
    static void set_red();
    static void set_green();
    static void set_blue();
    static void set_off();
    
private:
    static void put_pixel(uint32_t pixel_grb);
};

#endif // WS2812_LED_H_