
#include "position.h"
#include "quadrature_encoder.h"
#include <cstring>

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
    
    // Update positions from encoders before returning
    const_cast<Position*>(this)->update_from_encoders();
    
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

std::expected<void, Position::PositionError> 
Position::reset_encoder(size_t pos) noexcept {
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
