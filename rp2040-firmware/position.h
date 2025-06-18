#ifndef POSITION_H_
#define POSITION_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "quadrature_encoder.h"

class Position {
 private:
    bool initialized = false;
    void init();

    static constexpr size_t kPositions = QuadratureEncoder::kNumEncoders;
    std::array<double, kPositions> positions{};
    std::array<double, kPositions> scale_factors{};
    
    // Test mode functionality
    bool test_mode = false;
    uint32_t test_mode_start_time = 0;
    std::array<double, kPositions> test_mode_base_positions{};
    enum class TestPattern {
        SINE_WAVE,
        CIRCULAR,
        LINEAR_RAMP,
        RANDOM_WALK
    } test_pattern = TestPattern::SINE_WAVE;
    
    // Update positions from encoder counts
    void update_from_encoders();
    
    // Update positions in test mode
    void update_test_mode();

 public:
    enum class PositionError {
        InvalidIndex,
        NotInitialized,
        EncoderError
    };

    static Position& instance();

    // Returns true on success, false on error
    // Data format: [sentinel:4 bytes][positions:32 bytes] = 36 bytes total
    [[nodiscard]] bool get(uint8_t* out, size_t& bytes) const;

    void set(size_t pos, double value) {
        if (pos < kPositions) {
            positions[pos] = value;
        }
    }

    void set_scale(size_t pos, double scale) {
        if (pos < kPositions) {
            scale_factors[pos] = scale;
        }
    }
    
    double get_scale(size_t pos) const {
        if (pos < kPositions) {
            return scale_factors[pos];
        }
        return 1.0;
    }

    // Reset encoder at given position to zero
    // Returns true on success, false on error
    [[nodiscard]] bool reset_encoder(size_t pos);
    
    // Test mode control
    void enable_test_mode(bool enable);
    void set_test_pattern(uint8_t pattern);
    [[nodiscard]] bool is_test_mode() const { return test_mode; }

};

#endif  // #ifndef POSITION_H_
