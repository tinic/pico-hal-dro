#ifndef USB_DEVICE_H_
#define USB_DEVICE_H_

#include <cstddef>
#include <cstdint>

#include "tusb.h"

class USBDevice {
 public:
    static constexpr uint8_t VENDOR_INTERFACE = 0;
    static constexpr uint8_t EP_VENDOR_IN = 0x81;
    static constexpr uint8_t EP_VENDOR_OUT = 0x01;

    static constexpr uint16_t VENDOR_ID = 0x2E8A;   // Raspberry Pi vendor ID
    static constexpr uint16_t PRODUCT_ID = 0xC0DE;  // Custom product ID

    static constexpr uint8_t VENDOR_REQUEST_GET_POSITION = 0x01;
    static constexpr uint8_t VENDOR_REQUEST_ENABLE_TEST_MODE = 0x02;
    static constexpr uint8_t VENDOR_REQUEST_DISABLE_TEST_MODE = 0x03;
    static constexpr uint8_t VENDOR_REQUEST_SET_TEST_PATTERN = 0x04;
    
    enum class USBError {
        NotInitialized,
        TransmissionFailed,
        DeviceNotReady
    };

    static USBDevice& instance() noexcept;

    // Returns true on success, false on error
    [[nodiscard]] bool init() noexcept;
    void task() noexcept;
    // Returns true on success, false on error
    [[nodiscard]] bool send_position_data() noexcept;

 private:
    USBDevice() = default;
    bool initialized = false;
};

#endif  // USB_DEVICE_H_