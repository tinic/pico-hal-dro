#ifndef POSITION_H_
#define POSITION_H_

#include <array>
#include <cstdint>
#include <cstddef>
#include <expected>

class Position {
 private:
    bool initialized = false;
    void init() noexcept;

    static constexpr size_t kPositions = 4;
    std::array<double, kPositions> positions{};
    std::array<double, kPositions> scale_factors{1.0, 1.0, 1.0, 1.0};
    
    // Update positions from encoder counts
    void update_from_encoders() noexcept;

 public:
    enum class PositionError {
        InvalidIndex,
        NotInitialized,
        EncoderError
    };

    static Position& instance() noexcept;

    [[nodiscard]] std::expected<void, PositionError> get(uint8_t* out, size_t& bytes) const noexcept;
    
    void set(size_t pos, double value) noexcept { 
        if (pos < kPositions) {
            positions[pos] = value; 
        }
    }
    
    void set_scale(size_t pos, double scale) noexcept { 
        if (pos < kPositions) {
            scale_factors[pos] = scale; 
        }
    }
    
    // Reset encoder at given position to zero
    [[nodiscard]] std::expected<void, PositionError> reset_encoder(size_t pos) noexcept;

};


#endif  // #ifndef POSITION_H_
