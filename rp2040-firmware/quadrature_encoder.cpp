#include "quadrature_encoder.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "quadrature_encoder.pio.h"

QuadratureEncoder& QuadratureEncoder::instance() {
    static QuadratureEncoder encoder;
    if (!encoder.initialized) {
        encoder.init();
        encoder.initialized = true;
    }
    return encoder;
}

void QuadratureEncoder::init() {
    setup_pio();

    // Initialize timing for FIFO draining
    last_fifo_drain = to_ms_since_boot(get_absolute_time());

    // Initialize count offsets to zero
    count_offsets.fill(0);
    last_positions.fill(0);

    // Get initial PIO counts and reset them
    for (size_t i = 0; i < kNumEncoders; i++) {
        // Clear any garbage in the FIFOs first
        pio_sm_clear_fifos(pio, sm_nums[i]);
        
        // Wait a bit for the PIO to start pushing values
        sleep_us(100);
        
        // Get the initial count and use it as our zero reference
        int32_t initial_count = get_raw_count(i);
        count_offsets[i] = initial_count;
        last_positions[i] = initial_count;
    }
}

void QuadratureEncoder::setup_pio() {
    // Add the program to PIO instruction memory
    uint offset = pio_add_program(pio, &quadrature_encoder_program);

    // Initialize each encoder with its own state machine
    for (size_t i = 0; i < kNumEncoders; i++) {
        uint sm = pio_claim_unused_sm(pio, true);
        sm_nums[i] = sm;

        uint pin_base = kBasePin + (i * kPinsPerEncoder);
        quadrature_encoder_program_init(pio, sm, offset, pin_base, max_step_rate);
    }
}

int32_t QuadratureEncoder::get_raw_count(size_t encoder_idx) const {
    if (encoder_idx >= kNumEncoders) {
        return 0;
    }
    
    // Check if there are new values in the FIFO
    if (!pio_sm_is_rx_fifo_empty(pio, sm_nums[encoder_idx])) {
        // Read all available values to get the most recent
        int32_t count = last_positions[encoder_idx];
        while (!pio_sm_is_rx_fifo_empty(pio, sm_nums[encoder_idx])) {
            count = (int32_t)pio->rxf[sm_nums[encoder_idx]];
        }
        // Update our cached position
        last_positions[encoder_idx] = count;
        return count;
    } else {
        // No new data, return last known position
        return last_positions[encoder_idx];
    }
}

void QuadratureEncoder::update_fifo_drain() {
    // Temporarily disabled to debug hanging issue
    return;
}

bool QuadratureEncoder::get_all_counts(std::array<int64_t, kNumEncoders>& counts) const {
    if (!initialized) {
        return false;
    }

    // Fetch counts from all encoders
    for (size_t i = 0; i < kNumEncoders; i++) {
        int32_t raw_count = get_raw_count(i);
        // Simple subtraction without overflow handling for debugging
        counts[i] = static_cast<int32_t>(raw_count - count_offsets[i]);
    }

    return true;
}

bool QuadratureEncoder::get_count(size_t encoder_idx, int64_t& count) const {
    if (!initialized) {
        return false;
    }
    if (encoder_idx >= kNumEncoders) {
        return false;
    }

    // Get current raw count - no overflow handling for debugging
    int32_t raw_count = get_raw_count(encoder_idx);
    
    // Simple subtraction without overflow handling for debugging
    count = static_cast<int32_t>(raw_count - count_offsets[encoder_idx]);
    
    return true;
}

bool QuadratureEncoder::reset_count(size_t encoder_idx) {
    if (!initialized) {
        return false;
    }
    if (encoder_idx >= kNumEncoders) {
        return false;
    }

    // Get the current raw count
    int32_t raw_count = get_raw_count(encoder_idx);
    
    // Set this as the new zero reference
    count_offsets[encoder_idx] = raw_count;

    return true;
}