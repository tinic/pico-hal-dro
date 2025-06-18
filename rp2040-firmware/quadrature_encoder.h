#ifndef QUADRATURE_ENCODER_H_
#define QUADRATURE_ENCODER_H_

#include <array>
#include <cstddef>
#include <cstdint>

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
    void init();

    // Get current count for a specific encoder (64-bit)
    // Returns true on success, false on error. Count is returned via output parameter
    [[nodiscard]] bool get_count(size_t encoder_idx, int64_t& count) const;

    // Get counts for all encoders at once (more efficient)
    // Returns true on success, false on error. Counts are returned via output parameter
    [[nodiscard]] bool get_all_counts(std::array<int64_t, kNumEncoders>& counts) const;

    // Reset count for a specific encoder
    // Returns true on success, false on error
    [[nodiscard]] bool reset_count(size_t encoder_idx);

    // Set the maximum expected step rate (0 = no limit)
    constexpr void set_max_step_rate(int max_rate) {
        max_step_rate = max_rate;
    }

    // Periodic update to drain PIO FIFOs - should be called regularly
    void update_fifo_drain();

 private:
    QuadratureEncoder() = default;
    bool initialized = false;

    PIO pio = pio0;
    std::array<uint, kNumEncoders> sm_nums = {};

    // Simple 32-bit count offsets for zeroing
    std::array<int32_t, kNumEncoders> count_offsets = {};    // Reset offsets
    
    // Last known positions to maintain state when FIFO is empty
    mutable std::array<int32_t, kNumEncoders> last_positions = {};

    // FIFO drain timing
    uint32_t last_fifo_drain = 0;
    static constexpr uint32_t kFifoDrainInterval = 1000;  // Drain every 1ms

    int max_step_rate = 0;  // Maximum expected steps per second

    // Load the PIO program and configure state machines
    void setup_pio();
    
    // Get raw count from PIO, maintaining last known position
    [[nodiscard]] int32_t get_raw_count(size_t encoder_idx) const;
};

#endif  // QUADRATURE_ENCODER_H_