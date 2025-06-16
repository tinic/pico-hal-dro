#ifndef USB_DEVICE_H_
#define USB_DEVICE_H_

#include <cstdint>
#include <cstddef>
#include <expected>
#include "tusb.h"

class USBDevice {
public:
    static constexpr uint8_t VENDOR_INTERFACE = 0;
    static constexpr uint8_t EP_VENDOR_IN = 0x81;
    static constexpr uint8_t EP_VENDOR_OUT = 0x01;
    
    static constexpr uint16_t VENDOR_ID = 0x2E8A;  // Raspberry Pi vendor ID
    static constexpr uint16_t PRODUCT_ID = 0x10DF; // Custom product ID
    
    static constexpr uint8_t VENDOR_REQUEST_GET_POSITION = 0x01;
    static constexpr uint8_t VENDOR_REQUEST_START_STREAM = 0x02;
    static constexpr uint8_t VENDOR_REQUEST_STOP_STREAM = 0x03;
    static constexpr uint8_t VENDOR_REQUEST_ENABLE_TEST_MODE = 0x04;
    static constexpr uint8_t VENDOR_REQUEST_DISABLE_TEST_MODE = 0x05;
    static constexpr uint8_t VENDOR_REQUEST_SET_TEST_PATTERN = 0x06;
    
    enum class USBError {
        NotInitialized,
        TransmissionFailed,
        DeviceNotReady
    };
    
    static USBDevice& instance() noexcept;
    
    [[nodiscard]] std::expected<void, USBError> init() noexcept;
    void task() noexcept;
    [[nodiscard]] std::expected<void, USBError> send_position_data() noexcept;
    constexpr void enable_continuous_stream(bool enable) noexcept { continuous_stream = enable; }
    
private:
    USBDevice() = default;
    bool initialized = false;
    bool continuous_stream = false;
    uint32_t last_send_time = 0;
};

#endif // USB_DEVICE_H_