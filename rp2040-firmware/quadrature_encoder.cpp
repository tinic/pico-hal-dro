#include "quadrature_encoder.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "quadrature_encoder.pio.h"

QuadratureEncoder& QuadratureEncoder::instance() {
    static QuadratureEncoder encoder;
    return encoder;
}

std::expected<void, QuadratureEncoder::EncoderError> QuadratureEncoder::init() noexcept {
    if (initialized)
        return {};

    if (auto result = setup_pio(); !result) {
        return std::unexpected(result.error());
    }

    // Initialize timing for overflow checks
    last_overflow_check = to_ms_since_boot(get_absolute_time());

    // Initialize all counts to zero
    full_counts.fill(0);
    last_raw_counts.fill(0);
    count_offsets.fill(0);

    // Get initial PIO counts
    for (size_t i = 0; i < kNumEncoders; i++) {
        last_raw_counts[i] = quadrature_encoder_get_count(pio, sm_nums[i]);
    }

    initialized = true;
    return {};
}

std::expected<void, QuadratureEncoder::EncoderError> QuadratureEncoder::setup_pio() noexcept {
    // Add the program to PIO instruction memory
    uint offset = pio_add_program(pio, &quadrature_encoder_program);

    // Initialize each encoder with its own state machine
    for (size_t i = 0; i < kNumEncoders; i++) {
        uint sm = pio_claim_unused_sm(pio, true);
        if (sm == static_cast<uint>(-1)) {
            return std::unexpected(EncoderError::PIOError);
        }
        sm_nums[i] = sm;

        uint pin_base = kBasePin + (i * kPinsPerEncoder);
        quadrature_encoder_program_init(pio, sm, offset, pin_base, max_step_rate);
    }

    return {};
}

constexpr bool QuadratureEncoder::detect_overflow(int32_t old_count, int32_t new_count) noexcept {
    // Detect overflow by checking for large magnitude change and sign flip
    // This works because we expect small incremental changes, not huge jumps
    constexpr int32_t threshold = 0x40000000;  // Half of int32 range

    // Check for positive overflow (wrapped to negative)
    if (old_count > threshold && new_count < -threshold) {
        return true;  // Positive overflow
    }

    // Check for negative overflow (wrapped to positive)
    if (old_count < -threshold && new_count > threshold) {
        return true;  // Negative overflow
    }

    return false;
}

void QuadratureEncoder::update_encoder_overflow(size_t encoder_idx, int32_t raw_count) noexcept {
    int32_t old_count = last_raw_counts[encoder_idx];

    if (detect_overflow(old_count, raw_count)) {
        // Handle overflow
        if (old_count > 0 && raw_count < 0) {
            // Positive overflow: add 2^32 to maintain continuity
            full_counts[encoder_idx] += (1LL << 32);
        } else if (old_count < 0 && raw_count > 0) {
            // Negative overflow: subtract 2^32 to maintain continuity
            full_counts[encoder_idx] -= (1LL << 32);
        }
    }

    // Update the stored count
    last_raw_counts[encoder_idx] = raw_count;

    // Calculate the full 64-bit count
    // We need to handle the signed 32-bit value properly
    int64_t signed_raw = static_cast<int64_t>(raw_count);
    full_counts[encoder_idx] = (full_counts[encoder_idx] & 0xFFFFFFFF00000000LL) + signed_raw;
}

void QuadratureEncoder::update_overflow_check() noexcept {
    if (!initialized)
        return;

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_overflow_check < kOverflowCheckInterval) {
        return;  // Not time for check yet
    }

    last_overflow_check = now;

    // Request counts from all encoders
    for (size_t i = 0; i < kNumEncoders; i++) {
        quadrature_encoder_request_count(pio, sm_nums[i]);
    }

    // Process overflow for all encoders
    for (size_t i = 0; i < kNumEncoders; i++) {
        int32_t raw_count = quadrature_encoder_fetch_count(pio, sm_nums[i]);
        update_encoder_overflow(i, raw_count);
    }
}

std::expected<std::array<int64_t, QuadratureEncoder::kNumEncoders>, QuadratureEncoder::EncoderError>
QuadratureEncoder::get_all_counts() const noexcept {
    if (!initialized) {
        return std::unexpected(EncoderError::NotInitialized);
    }

    std::array<int64_t, kNumEncoders> counts{};

    // Request counts from all encoders
    for (size_t i = 0; i < kNumEncoders; i++) {
        quadrature_encoder_request_count(pio, sm_nums[i]);
    }

    // Fetch and process counts from all encoders
    for (size_t i = 0; i < kNumEncoders; i++) {
        int32_t raw_count = quadrature_encoder_fetch_count(pio, sm_nums[i]);
        const_cast<QuadratureEncoder*>(this)->update_encoder_overflow(i, raw_count);
        counts[i] = full_counts[i] - count_offsets[i];
    }

    return counts;
}

std::expected<int64_t, QuadratureEncoder::EncoderError> QuadratureEncoder::get_count(
    size_t encoder_idx) const noexcept {
    if (!initialized) {
        return std::unexpected(EncoderError::NotInitialized);
    }
    if (encoder_idx >= kNumEncoders) {
        return std::unexpected(EncoderError::InvalidIndex);
    }

    // Get current raw count and update overflow detection
    int32_t raw_count = quadrature_encoder_get_count(pio, sm_nums[encoder_idx]);
    const_cast<QuadratureEncoder*>(this)->update_encoder_overflow(encoder_idx, raw_count);

    // Return the 64-bit count minus offset
    return full_counts[encoder_idx] - count_offsets[encoder_idx];
}

std::expected<void, QuadratureEncoder::EncoderError> QuadratureEncoder::reset_count(size_t encoder_idx) noexcept {
    if (!initialized) {
        return std::unexpected(EncoderError::NotInitialized);
    }
    if (encoder_idx >= kNumEncoders) {
        return std::unexpected(EncoderError::InvalidIndex);
    }

    // Update overflow status first
    int32_t raw_count = quadrature_encoder_get_count(pio, sm_nums[encoder_idx]);
    update_encoder_overflow(encoder_idx, raw_count);

    // Set the current full count as the offset
    count_offsets[encoder_idx] = full_counts[encoder_idx];

    return {};
}