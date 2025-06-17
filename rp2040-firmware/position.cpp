
#include "position.h"
#include "quadrature_encoder.h"
#include "usb_device.h"
#include "pico/time.h"
#include <cstring>
#include <cmath>
#include <cstdlib>

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
    if (!QuadratureEncoder::instance().init()) {
        // Encoder initialization failed - this is a critical error
        // Set initialized to false to indicate system is not ready
        initialized = false;
        return;
    }
    initialized = true;
}

bool Position::get(uint8_t* out, size_t& bytes) const noexcept {
    if (!initialized) {
        return false;
    }
    
    // Update positions from encoders or test mode before returning
    if (test_mode) {
        const_cast<Position*>(this)->update_test_mode();
    } else {
        const_cast<Position*>(this)->update_from_encoders();
    }
    
    // Data format: [sentinel:4 bytes][positions:32 bytes] = 36 bytes total
    bytes = sizeof(uint32_t) + sizeof(positions);
    if (out != nullptr) {
        // Write sentinel first
        uint32_t sentinel = USBDevice::POSITION_DATA_SENTINEL;
        memcpy(out, &sentinel, sizeof(sentinel));
        
        // Write position data after sentinel
        memcpy(out + sizeof(sentinel), reinterpret_cast<const uint8_t*>(positions.data()), sizeof(positions));
    }

    return true;
}

void Position::update_from_encoders() noexcept {
    std::array<int64_t, kPositions> counts;
    if (QuadratureEncoder::instance().get_all_counts(counts)) {
        // Convert encoder counts to position values using scale factors
        for (size_t i = 0; i < kPositions; i++) {
            positions[i] = static_cast<double>(counts[i]) * scale_factors[i];
        }
    }
    // If get_all_counts fails, keep previous position values - this provides
    // graceful degradation rather than complete failure
}

bool Position::reset_encoder(size_t pos) noexcept {
    if (!initialized) {
        return false;
    }
    if (pos >= kPositions) {
        return false;
    }

    // Reset the encoder count and position value
    if (!QuadratureEncoder::instance().reset_count(pos)) {
        return false;
    }

    positions[pos] = 0.0;
    return true;
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
            static uint32_t last_update = 0;
            static uint32_t seed = 0x12345678;
            
            if (now - last_update >= 50) { // Update every 50ms
                // Simple linear congruential generator
                auto next_random = [](uint32_t& s) -> double {
                    s = (s * 1664525u + 1013904223u);
                    return ((s >> 16) & 0x7FFF) / 32768.0 - 0.5;  // Range -0.5 to 0.5
                };
                
                positions[0] += next_random(seed) * 0.02;
                positions[1] += next_random(seed) * 0.02;
                positions[2] += next_random(seed) * 0.01;
                positions[3] += next_random(seed) * 0.1;
                last_update = now;
            }
            break;
        }
    }
}
