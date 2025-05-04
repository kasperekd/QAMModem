#pragma once

#include <cmath>
#include <concepts>
#include <memory_resource>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

/**
 * @brief Template class for QAM (Quadrature Amplitude Modulation) modulator.
 *
 * This class supports QPSK (4-QAM), 16-QAM, and 64-QAM modulation schemes.
 * It maps a sequence of bits to complex symbols based on the constellation
 * diagram.
 *
 * @tparam Levels Number of constellation points (must be 4, 16, or 64)
 * @tparam T      Numeric type used for representing constellation coordinates
 */
template <int Levels, Numeric T>
class ModulatorQAM {
   public:
    /// @brief Type used for numeric representation of constellation points
    using value_type = T;

    /// @brief Number of bits encoded in each symbol
    static constexpr int BitsPerSymbol = std::log2(Levels);

    /**
     * @brief Construct a new ModulatorQAM object
     *
     * @param scale_factor Scaling factor applied to all constellation points
     * @param resource Memory resource for memory allocation
     */
    explicit ModulatorQAM(
        T scale_factor = T(1),
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : alloc_(resource), scale_factor_(scale_factor) {
        static_assert(
            Levels == 4 || Levels == 16 || Levels == 64,
            "Only QPSK (4), 16QAM (16) and 64QAM (64) are supported.");
        generateConstellation();
    }

    /**
     * @brief Modulate a sequence of bits into complex symbols
     *
     * @param bits Input bit stream as a span of bytes
     * @return Vector of modulated symbols represented as (real, imaginary)
     * pairs
     * @throws std::invalid_argument if number of bits is not divisible by
     * BitsPerSymbol
     */
    std::pmr::vector<std::pair<T, T>> modulate(
        std::span<const uint8_t> bits) const {
        if (bits.size() % BitsPerSymbol != 0)
            throw std::invalid_argument(
                "Bit count must be divisible by BitsPerSymbol");

        size_t num_symbols = bits.size() / BitsPerSymbol;
        std::pmr::vector<std::pair<T, T>> symbols(num_symbols, alloc_);

        for (size_t i = 0; i < num_symbols; ++i) {
            uint8_t index = 0;
            for (int j = 0; j < BitsPerSymbol; ++j) {
                index = (index << 1) | bits[i * BitsPerSymbol + j];
            }
            symbols[i] = constellation_[index];
        }

        return symbols;
    }

    /**
     * @brief Get the average power of the constellation
     *
     * @return Average power (mean squared magnitude of constellation points)
     */
    T getAveragePower() const { return avg_power_; }

   private:
    /**
     * @brief Generate the constellation diagram based on the specified
     * modulation scheme
     */
    void generateConstellation() {
        if constexpr (Levels == 4) {  // QPSK
            constellation_ = {{+1, +1}, {-1, +1}, {-1, -1}, {+1, -1}};
        } else if constexpr (Levels == 16) {  // 16QAM
            for (int i = -3; i <= 3; i += 2)
                for (int j = -3; j <= 3; j += 2)
                    constellation_.emplace_back(i, j);
        } else if constexpr (Levels == 64) {  // 64QAM
            for (int i = -7; i <= 7; i += 2)
                for (int j = -7; j <= 7; j += 2)
                    constellation_.emplace_back(i, j);
        }

        for (auto& [re, im] : constellation_) {
            re *= scale_factor_;
            im *= scale_factor_;
        }

        avg_power_ = 0;
        for (const auto& [re, im] : constellation_) {
            avg_power_ += re * re + im * im;
        }
        avg_power_ /= Levels;
    }

    std::pmr::polymorphic_allocator<T> alloc_;
    std::pmr::vector<std::pair<T, T>> constellation_;
    T avg_power_;
    T scale_factor_;
};