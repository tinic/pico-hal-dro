#include "usb_device.h"

#include <array>
#include <cstring>

#include "hardware/gpio.h"
#include "pico/time.h"
#include "position.h"
#include "tusb.h"
#include "version.h"
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
    TUD_VENDOR_DESCRIPTOR(USBDevice::VENDOR_INTERFACE, 0, USBDevice::EP_VENDOR_OUT, USBDevice::EP_VENDOR_IN, 64)};

// String descriptors
char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04},                       // 0: is supported language is English (0x0409)
    "RP2040",                                         // 1: Manufacturer
    "Quadrature Encoder",                             // 2: Product
    "4ENC-" GIT_SHORT_SHA "-" GIT_COMMIT_DATE_SHORT,  // 3: Serial
};

USBDevice& USBDevice::instance() {
    static USBDevice device;
    return device;
}

bool USBDevice::init() noexcept {
    if (initialized)
        return true;

    tusb_init();
    initialized = true;

    return true;
}

void USBDevice::task() noexcept {
    tud_task();

    // Check if we received a request
    while (tud_vendor_n_available(VENDOR_INTERFACE)) {
        std::array<uint8_t, 64> request_buf{};  // Larger buffer
        uint32_t count = tud_vendor_n_read(VENDOR_INTERFACE, request_buf.data(), request_buf.size());
        if (count > 0) {
            static bool flip = false;
            if (flip) {
                WS2812Led::set_color(64, 64, 0);  // Yellow
            } else {
                WS2812Led::set_off();
            }
            flip ^= 1;
            // Process each byte as a potential command
            for (uint32_t i = 0; i < count; i++) {
                switch (request_buf[i]) {
                    case VENDOR_REQUEST_GET_POSITION:
                        (void)send_position_data();
                        break;
                    case VENDOR_REQUEST_SET_TEST_MODE:
                        if (i + 1 < count) {
                            uint8_t mode = request_buf[i + 1];
                            if (mode == 0) {
                                Position::instance().enable_test_mode(false);
                            } else {
                                Position::instance().enable_test_mode(true);
                                Position::instance().set_test_pattern(mode - 1);  // Convert 1-4 to 0-3
                            }
                            i++;  // Skip the parameter byte
                        }
                        break;
                    case VENDOR_REQUEST_SET_SCALE:
                        // Expect: encoder_index (1 byte) + scale (8 bytes double)
                        if (i + 9 <= count) {
                            uint8_t encoder_index = request_buf[i + 1];
                            double scale;
                            memcpy(&scale, &request_buf[i + 2], sizeof(double));
                            if (encoder_index < 4) {
                                Position::instance().set_scale(encoder_index, scale);
                            }
                            i += 9;  // Skip the parameter bytes
                        }
                        break;
                    case VENDOR_REQUEST_GET_SCALE:
                        (void)send_scale_data();
                        break;
                    case VENDOR_REQUEST_RESET_ENCODER:
                        if (i + 1 < count) {
                            uint8_t encoder_index = request_buf[i + 1];
                            if (encoder_index < 4) {
                                (void)Position::instance().reset_encoder(encoder_index);
                            }
                            i++;  // Skip the parameter byte
                        }
                        break;
                }
            }
        }
    }
}

bool USBDevice::send_position_data() noexcept {
    if (!initialized) {
        return false;
    }

    // Wait until ready
    if (!tud_vendor_n_mounted(VENDOR_INTERFACE)) {
        return false;
    }

    static std::array<uint8_t, 64> buffer{};
    size_t bytes = 0;

    if (!Position::instance().get(buffer.data(), bytes)) {
        return false;
    }

    // Send the position data
    uint32_t written = tud_vendor_n_write(VENDOR_INTERFACE, buffer.data(), bytes);
    if (bytes != written) {
        return false;
    }
    return true;
}

bool USBDevice::send_scale_data() noexcept {
    if (!initialized) {
        return false;
    }

    // Wait until ready
    if (!tud_vendor_n_mounted(VENDOR_INTERFACE)) {
        return false;
    }

    // Prepare scale data buffer: [sentinel:4 bytes][scales:32 bytes] = 36 bytes total
    static std::array<uint8_t, 36> buffer{};
    Position& pos = Position::instance();
    
    // Write sentinel first
    uint32_t sentinel = SCALE_DATA_SENTINEL;
    memcpy(buffer.data(), &sentinel, sizeof(sentinel));
    
    // Write scale data after sentinel
    for (size_t i = 0; i < 4; i++) {
        double scale = pos.get_scale(i);
        memcpy(&buffer[sizeof(sentinel) + i * sizeof(double)], &scale, sizeof(double));
    }

    // Send the data
    uint32_t written = tud_vendor_n_write(VENDOR_INTERFACE, buffer.data(), buffer.size());
    if (buffer.size() != written) {
        return false;
    }

    return true;
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
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
    (void)itf;
    (void)sent_bytes;
}

}  // extern "C"