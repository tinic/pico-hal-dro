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
    static constexpr uint8_t VENDOR_REQUEST_SET_TEST_MODE = 0x02;  // 0=off, 1-4=test patterns
    static constexpr uint8_t VENDOR_REQUEST_SET_SCALE = 0x03;      // Set scale factor for an encoder
    static constexpr uint8_t VENDOR_REQUEST_GET_SCALE = 0x04;      // Get all scale factors
    static constexpr uint8_t VENDOR_REQUEST_RESET_ENCODER = 0x05;  // Reset encoder position to zero
    
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
    [[nodiscard]] bool send_scale_data() noexcept;

 private:
    USBDevice() = default;
    bool initialized = false;
};

#endif  // USB_DEVICE_H_