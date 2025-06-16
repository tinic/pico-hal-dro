#ifndef USB_DEVICE_H_
#define USB_DEVICE_H_

#include <cstddef>
#include <cstdint>
#include <expected>

#include "tusb.h"

class USBDevice {
 public:
    static constexpr uint8_t VENDOR_INTERFACE = 0;
    static constexpr uint8_t EP_VENDOR_IN = 0x81;
    static constexpr uint8_t EP_VENDOR_OUT = 0x01;

    static constexpr uint16_t VENDOR_ID = 0x2E8A;   // Raspberry Pi vendor ID
    static constexpr uint16_t PRODUCT_ID = 0x10DF;  // Custom product ID

    static constexpr uint8_t VENDOR_REQUEST_GET_POSITION = 0x01;

    enum class USBError {
        NotInitialized,
        TransmissionFailed,
        DeviceNotReady
    };

    static USBDevice& instance() noexcept;

    [[nodiscard]] std::expected<void, USBError> init() noexcept;
    void task() noexcept;
    [[nodiscard]] std::expected<void, USBError> send_position_data() noexcept;

 private:
    USBDevice() = default;
    bool initialized = false;
};

#endif  // USB_DEVICE_H_