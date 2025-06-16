#include "usb_device.h"

#include <array>
#include <cstring>

#include "hardware/gpio.h"
#include "pico/time.h"
#include "position.h"
#include "tusb.h"

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

// Configuration descriptor
uint8_t const desc_configuration[] = {
    // Configuration Descriptor
    9,                                   // bLength
    TUSB_DESC_CONFIGURATION,             // bDescriptorType
    TUD_CONFIG_DESC_LEN + 9 + 7 + 7,     // wTotalLength
    1,                                   // bNumInterfaces
    1,                                   // bConfigurationValue
    0,                                   // iConfiguration
    TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,  // bmAttributes
    100,                                 // bMaxPower (100 mA)

    // Interface Descriptor
    9,                            // bLength
    TUSB_DESC_INTERFACE,          // bDescriptorType
    USBDevice::VENDOR_INTERFACE,  // bInterfaceNumber
    0,                            // bAlternateSetting
    2,                            // bNumEndpoints
    0xFF,                         // bInterfaceClass (Vendor Specific)
    0x00,                         // bInterfaceSubClass
    0x00,                         // bInterfaceProtocol
    0,                            // iInterface

    // Endpoint Descriptor (IN)
    7,                        // bLength
    TUSB_DESC_ENDPOINT,       // bDescriptorType
    USBDevice::EP_VENDOR_IN,  // bEndpointAddress
    TUSB_XFER_BULK,           // bmAttributes
    64,                       // wMaxPacketSize
    0,                        // bInterval

    // Endpoint Descriptor (OUT)
    7,                         // bLength
    TUSB_DESC_ENDPOINT,        // bDescriptorType
    USBDevice::EP_VENDOR_OUT,  // bEndpointAddress
    TUSB_XFER_BULK,            // bmAttributes
    64,                        // wMaxPacketSize
    0,                         // bInterval
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
    if (tud_vendor_n_available(VENDOR_INTERFACE)) {
        uint8_t request_buf[2];
        uint32_t count = tud_vendor_n_read(VENDOR_INTERFACE, &request_buf, 2);
        if (count > 0) {
            switch (request_buf[0]) {
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
                    if (count >= 2) {
                        Position::instance().set_test_pattern(request_buf[1]);
                    }
                    break;
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

    // Blink LED to indicate USB traffic
    gpio_put(PICO_DEFAULT_LED_PIN, true);

    // Send the position data
    if (tud_vendor_n_write(VENDOR_INTERFACE, buffer.data(), bytes) != bytes) {
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        return std::unexpected(USBError::TransmissionFailed);
    }

    gpio_put(PICO_DEFAULT_LED_PIN, false);
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

    // Blink LED to indicate USB RX traffic
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    sleep_us(100);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
    (void)itf;
    (void)sent_bytes;
}

}  // extern "C"