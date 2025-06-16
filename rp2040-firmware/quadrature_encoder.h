#ifndef QUADRATURE_ENCODER_H_
#define QUADRATURE_ENCODER_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>

#include "hardware/pio.h"
#include "pico/time.h"

class QuadratureEncoder {
 public:
    static constexpr size_t kNumEncoders = 4;

    // Pin pairs for each encoder (A, B)
    // Encoder 0: pins 0, 1
    // Encoder 1: pins 2, 3
    // Encoder 2: pins 4, 5
    // Encoder 3: pins 6, 7
    static constexpr uint kBasePin = 0;
    static constexpr uint kPinsPerEncoder = 2;

    enum class EncoderError {
        InvalidIndex,
        NotInitialized,
        PIOError
    };

    static QuadratureEncoder& instance();

    // Initialize all encoders
    [[nodiscard]] std::expected<void, EncoderError> init() noexcept;

    // Get current count for a specific encoder (64-bit)
    [[nodiscard]] std::expected<int64_t, EncoderError> get_count(size_t encoder_idx) const noexcept;

    // Get counts for all encoders at once (more efficient)
    [[nodiscard]] std::expected<std::array<int64_t, kNumEncoders>, EncoderError> get_all_counts() const noexcept;

    // Reset count for a specific encoder
    [[nodiscard]] std::expected<void, EncoderError> reset_count(size_t encoder_idx) noexcept;

    // Set the maximum expected step rate (0 = no limit)
    constexpr void set_max_step_rate(int max_rate) noexcept {
        max_step_rate = max_rate;
    }

    // Periodic update to check for overflows - should be called regularly
    void update_overflow_check() noexcept;

 private:
    QuadratureEncoder() = default;
    bool initialized = false;

    PIO pio = pio0;
    std::array<uint, kNumEncoders> sm_nums = {};

    // 64-bit counters and overflow handling
    std::array<int64_t, kNumEncoders> full_counts = {};      // Full 64-bit counts
    std::array<int32_t, kNumEncoders> last_raw_counts = {};  // Last 32-bit PIO counts
    std::array<int64_t, kNumEncoders> count_offsets = {};    // Reset offsets

    // Overflow detection timing
    uint32_t last_overflow_check = 0;
    static constexpr uint32_t kOverflowCheckInterval = 1000;  // Check every 1ms

    int max_step_rate = 0;  // Maximum expected steps per second

    // Load the PIO program and configure state machines
    [[nodiscard]] std::expected<void, EncoderError> setup_pio() noexcept;

    // Update a single encoder's overflow status
    void update_encoder_overflow(size_t encoder_idx, int32_t raw_count) noexcept;

    // Detect if overflow occurred based on sign change
    [[nodiscard]] static constexpr bool detect_overflow(int32_t old_count, int32_t new_count) noexcept;
};

#endif  // QUADRATURE_ENCODER_H_