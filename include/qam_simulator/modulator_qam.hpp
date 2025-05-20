#pragma once

#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * @brief Class for QAM (Quadrature Amplitude Modulation) modulator.
 *
 * This class supports QPSK (4-QAM), 16-QAM, and 64-QAM modulation schemes.
 * It maps a sequence of bits to complex symbols based on the constellation
 * diagram using Gray coding to minimize bit errors.
 */
class ModulatorQAM {
   public:
    /// @brief Type used for numeric representation of constellation points
    using value_type = float;

    /**
     * @brief Construct a new ModulatorQAM object
     *
     * @param levels_in Number of constellation points (must be 4, 16, or 64).
     * @param scale_factor Scaling factor applied to all constellation points
     */
    explicit ModulatorQAM(int levels_in, value_type scale_factor = 1.0f)
        : levels_count_(levels_in),
          bits_per_symbol_(calculate_bits_per_symbol(levels_in)),
          scale_factor_(scale_factor) {
        if (levels_count_ != 4 && levels_count_ != 16 && levels_count_ != 64) {
            throw std::invalid_argument(
                "ModulatorQAM: Only 4, 16, and 64 QAM levels are supported");
        }
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
    std::vector<std::pair<value_type, value_type>> modulate(
        std::span<const uint8_t> bits) const {
        if (bits.size() % bits_per_symbol_ != 0) {
            throw std::invalid_argument(
                "Bit count must be divisible by BitsPerSymbol");
        }
        if (bits.empty()) {
            return {};
        }

        size_t num_symbols = bits.size() / bits_per_symbol_;
        std::vector<std::pair<value_type, value_type>> symbols(num_symbols);

        for (size_t i = 0; i < num_symbols; ++i) {
            int symbol_index = 0;
            for (int j = 0; j < bits_per_symbol_; ++j) {
                symbol_index =
                    (symbol_index << 1) | bits[i * bits_per_symbol_ + j];
            }
            symbols[i] = constellation_[symbol_index];
        }
        return symbols;
    }

    /**
     * @brief Get the average power of the constellation
     */
    value_type getAveragePower() const { return avg_power_; }

    /**
     * @brief Get the number of bits encoded in each symbol
     */
    constexpr int getBitsPerSymbol() const noexcept { return bits_per_symbol_; }

    /**
     * @brief Get the number of constellation points
     */
    constexpr int getLevelsCount() const noexcept { return levels_count_; }

   private:
    static constexpr int calculate_bits_per_symbol(int levels) {
        if (levels == 4) return 2;
        if (levels == 16) return 4;
        if (levels == 64) return 6;
        throw std::logic_error(
            "Invalid levels value for BitsPerSymbol calculation");
    }

    void generateConstellation() {
        constellation_.clear();
        constellation_.reserve(levels_count_);

        if (levels_count_ == 4) {  // QPSK
            constellation_.emplace_back(1.0f, 1.0f);
            constellation_.emplace_back(-1.0f, 1.0f);
            constellation_.emplace_back(-1.0f, -1.0f);
            constellation_.emplace_back(1.0f, -1.0f);
        } else if (levels_count_ == 16) {  // 16-QAM
            const value_type pam_levels_arr[] = {-3.0f, -1.0f, 1.0f, 3.0f};
            for (int index = 0; index < levels_count_; ++index) {
                int val_x_bits = (index >> (bits_per_symbol_ / 2));
                int val_y_bits = (index & ((1 << (bits_per_symbol_ / 2)) - 1));
                int gray_idx_x = val_x_bits ^ (val_x_bits >> 1);
                int gray_idx_y = val_y_bits ^ (val_y_bits >> 1);
                constellation_.emplace_back(pam_levels_arr[gray_idx_x],
                                            pam_levels_arr[gray_idx_y]);
            }
        } else if (levels_count_ == 64) {  // 64-QAM
            const value_type pam_levels_arr[] = {-7.0f, -5.0f, -3.0f, -1.0f,
                                                 1.0f,  3.0f,  5.0f,  7.0f};
            for (int index = 0; index < levels_count_; ++index) {
                int val_x_bits = (index >> (bits_per_symbol_ / 2));
                int val_y_bits = (index & ((1 << (bits_per_symbol_ / 2)) - 1));
                int gray_idx_x = val_x_bits ^ (val_x_bits >> 1);
                int gray_idx_y = val_y_bits ^ (val_y_bits >> 1);
                constellation_.emplace_back(pam_levels_arr[gray_idx_x],
                                            pam_levels_arr[gray_idx_y]);
            }
        }

        avg_power_ = 0.0f;
        for (auto& point : constellation_) {
            point.first *= scale_factor_;
            point.second *= scale_factor_;
            avg_power_ +=
                point.first * point.first + point.second * point.second;
        }
        if (levels_count_ > 0) {
            avg_power_ /= static_cast<value_type>(levels_count_);
        } else {
            avg_power_ = 0.0f;
        }
    }

    const int levels_count_;
    const int bits_per_symbol_;
    const value_type scale_factor_;

    std::vector<std::pair<value_type, value_type>> constellation_;
    value_type avg_power_;
};