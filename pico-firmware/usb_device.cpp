#include "usb_device.h"

#include <array>
#include <cstring>

#include "hardware/gpio.h"
#include "pico/time.h"
#include "position.h"
#include "tusb.h"
#include "ws2812_led.h"

// USB Descriptors
tusb_desc_device_t const desc_device = {.bLength = sizeof(tusb_desc_device_t),
                                        .bDescriptorType = TUSB_DESC_DEVICE,
                                        .bcdUSB = 0x0200,
                                        .bDeviceClass = 0xFF,  // Vendor Specific Class
                                        .bDeviceSubClass = 0x00,
                                        .bDeviceProtocol = 0x00,
                                        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
                                        .idVendor = USBDevice::VENDOR_ID,
                                        .idProduct = USBDevice::PRODUCT_ID,
                                        .bcdDevice = 0x0100,
                                        .iManufacturer = 0x01,
                                        .iProduct = 0x02,
                                        .iSerialNumber = 0x03,
                                        .bNumConfigurations = 0x01};

// Configuration descriptor using TinyUSB macros
uint8_t const desc_configuration[] = {
    // Config: self powered with remote wakeup, max power 100mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    
    // Interface: vendor specific class
    TUD_VENDOR_DESCRIPTOR(USBDevice::VENDOR_INTERFACE, 0, USBDevice::EP_VENDOR_OUT, USBDevice::EP_VENDOR_IN, 64)
};

// String descriptors
char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "Raspberry Pi",              // 1: Manufacturer
    "Pico HAL DRO Device",       // 2: Product
    "1234567890",                // 3: Serial
};

USBDevice& USBDevice::instance() {
    static USBDevice device;
    return device;
}

std::expected<void, USBDevice::USBError> USBDevice::init() noexcept {
    if (initialized)
        return {};

    tusb_init();
    initialized = true;

    return {};
}

void USBDevice::task() noexcept {
    tud_task();

    // Check if we received a request
    while (tud_vendor_n_available(VENDOR_INTERFACE)) {
        uint8_t request_buf[64];  // Larger buffer
        uint32_t count = tud_vendor_n_read(VENDOR_INTERFACE, request_buf, sizeof(request_buf));
        if (count > 0) {
            // Yellow flash to indicate command received (more distinct)
            WS2812Led::set_color(255, 255, 0);  // Yellow
            sleep_us(50000);  // 50ms
            WS2812Led::set_off();
            
            // Process each byte as a potential command
            for (uint32_t i = 0; i < count; i++) {
                switch (request_buf[i]) {
                    case VENDOR_REQUEST_GET_POSITION:
                        (void)send_position_data();  // Explicitly ignore result for now
                        break;
                    case VENDOR_REQUEST_ENABLE_TEST_MODE:
                        Position::instance().enable_test_mode(true);
                        break;
                    case VENDOR_REQUEST_DISABLE_TEST_MODE:
                        Position::instance().enable_test_mode(false);
                        break;
                    case VENDOR_REQUEST_SET_TEST_PATTERN:
                        if (i + 1 < count) {
                            Position::instance().set_test_pattern(request_buf[i + 1]);
                            i++; // Skip the parameter byte
                        }
                        break;
                }
            }
        }
    }
}

std::expected<void, USBDevice::USBError> USBDevice::send_position_data() noexcept {
    if (!initialized) {
        return std::unexpected(USBError::NotInitialized);
    }

    if (!tud_ready()) {
        return std::unexpected(USBError::DeviceNotReady);
    }

    std::array<uint8_t, 64> buffer{};
    size_t bytes = 0;

    if (auto result = Position::instance().get(buffer.data(), bytes); !result) {
        return std::unexpected(USBError::TransmissionFailed);
    }

    // Red LED to indicate sending response
    WS2812Led::set_red();

    // Send the position data
    uint32_t written = tud_vendor_n_write(VENDOR_INTERFACE, buffer.data(), bytes);
    
    // Keep LED on for a visible duration
    sleep_us(100000);  // 100ms
    WS2812Led::set_off();
    
    if (written != bytes) {
        return std::unexpected(USBError::TransmissionFailed);
    }
    return {};
}

// TinyUSB callbacks
extern "C" {

uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*)&desc_device;
}

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    static uint16_t _desc_str[32];
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
            return NULL;

        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31)
            chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}

// Vendor class callbacks
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
    // Nothing to handle for now
    (void)rhport;
    (void)stage;
    (void)request;
    return false;
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize) {
    (void)itf;
    (void)buffer;
    (void)bufsize;

    // Purple flash to indicate USB RX traffic (more distinct)
    WS2812Led::set_color(128, 0, 128);  // Purple
    sleep_us(30000);  // 30ms
    WS2812Led::set_off();
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
    (void)itf;
    (void)sent_bytes;
}

}  // extern "C"