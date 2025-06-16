
#include "position.h"
#include "quadrature_encoder.h"
#include "pico/time.h"
#include <cstring>
#include <cmath>
#include <random>

Position& Position::instance() {
    static Position position;
    if (!position.initialized) {
        position.initialized = true;
        position.init();
    }
    return position;
}

void Position::init() noexcept {
    // Initialize the quadrature encoder system
    if (auto result = QuadratureEncoder::instance().init(); !result) {
        // Encoder initialization failed - this is a critical error
        // Set initialized to false to indicate system is not ready
        initialized = false;
        return;
    }
    initialized = true;
}

std::expected<void, Position::PositionError> Position::get(uint8_t* out, size_t& bytes) const noexcept {
    if (!initialized) {
        return std::unexpected(PositionError::NotInitialized);
    }
    
    // Update positions from encoders or test mode before returning
    if (test_mode) {
        const_cast<Position*>(this)->update_test_mode();
    } else {
        const_cast<Position*>(this)->update_from_encoders();
    }
    
    bytes = sizeof(positions);
    if (out != nullptr) {
        memcpy(out, reinterpret_cast<const uint8_t*>(positions.data()), sizeof(positions));
    }

    return {};
}

void Position::update_from_encoders() noexcept {
    if (auto result = QuadratureEncoder::instance().get_all_counts(); result) {
        const auto& counts = *result;

        // Convert encoder counts to position values using scale factors
        for (size_t i = 0; i < kPositions; i++) {
            positions[i] = static_cast<double>(counts[i]) * scale_factors[i];
        }
    }
    // If get_all_counts fails, keep previous position values - this provides
    // graceful degradation rather than complete failure
}

std::expected<void, Position::PositionError> Position::reset_encoder(size_t pos) noexcept {
    if (!initialized) {
        return std::unexpected(PositionError::NotInitialized);
    }
    if (pos >= kPositions) {
        return std::unexpected(PositionError::InvalidIndex);
    }

    // Reset the encoder count and position value
    if (auto result = QuadratureEncoder::instance().reset_count(pos); !result) {
        return std::unexpected(PositionError::EncoderError);
    }

    positions[pos] = 0.0;
    return {};
}

void Position::enable_test_mode(bool enable) noexcept {
    if (enable && !test_mode) {
        test_mode = true;
        test_mode_start_time = to_ms_since_boot(get_absolute_time());
        test_mode_base_positions = positions;
    } else if (!enable && test_mode) {
        test_mode = false;
    }
}

void Position::set_test_pattern(uint8_t pattern) noexcept {
    if (pattern < 4) {
        test_pattern = static_cast<TestPattern>(pattern);
        if (test_mode) {
            test_mode_start_time = to_ms_since_boot(get_absolute_time());
            test_mode_base_positions = positions;
        }
    }
}

void Position::update_test_mode() noexcept {
    if (!test_mode) return;
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint32_t elapsed = now - test_mode_start_time;
    double time_sec = elapsed * 0.001;
    
    switch (test_pattern) {
        case TestPattern::SINE_WAVE: {
            positions[0] = test_mode_base_positions[0] + 5.0 * sin(time_sec * 0.5);         // X: 5mm amplitude, 0.5 rad/s
            positions[1] = test_mode_base_positions[1] + 3.0 * sin(time_sec * 0.7 + 1.57); // Y: 3mm amplitude, 0.7 rad/s, 90° phase
            positions[2] = test_mode_base_positions[2] + 2.0 * sin(time_sec * 0.3);         // Z: 2mm amplitude, 0.3 rad/s
            positions[3] = test_mode_base_positions[3] + 45.0 * sin(time_sec * 0.2);        // A: 45° amplitude, 0.2 rad/s
            break;
        }
        
        case TestPattern::CIRCULAR: {
            double radius = 10.0;
            double angular_vel = 0.3;
            positions[0] = test_mode_base_positions[0] + radius * cos(time_sec * angular_vel);
            positions[1] = test_mode_base_positions[1] + radius * sin(time_sec * angular_vel);
            positions[2] = test_mode_base_positions[2] + 1.0 * sin(time_sec * 0.1);
            positions[3] = test_mode_base_positions[3] + time_sec * 5.0;
            break;
        }
        
        case TestPattern::LINEAR_RAMP: {
            positions[0] = test_mode_base_positions[0] + time_sec * 2.0;   // 2 mm/s
            positions[1] = test_mode_base_positions[1] + time_sec * 1.5;   // 1.5 mm/s
            positions[2] = test_mode_base_positions[2] + time_sec * 0.5;   // 0.5 mm/s
            positions[3] = test_mode_base_positions[3] + time_sec * 10.0;  // 10 deg/s
            break;
        }
        
        case TestPattern::RANDOM_WALK: {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::normal_distribution<double> dis(0.0, 0.01);
            static uint32_t last_update = 0;
            
            if (now - last_update >= 50) { // Update every 50ms
                positions[0] += dis(gen);
                positions[1] += dis(gen);
                positions[2] += dis(gen) * 0.5;
                positions[3] += dis(gen) * 5.0;
                last_update = now;
            }
            break;
        }
    }
}
