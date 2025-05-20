#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

/**
 * @brief Class for QAM (Quadrature Amplitude Modulation) demodulator.
 *
 * This class supports hard demodulation for QPSK (4-QAM), 16-QAM, and
 * 64-QAM. It maps received complex symbols back to bit sequences using
 * constellation matching.
 */
class DemodulatorQAM {
   public:
    /// @brief Type used for numeric representation of constellation points
    using value_type = float;

    /**
     * @brief Construct a new DemodulatorQAM object
     *
     * @param levels_in Number of constellation points (must be 4, 16, or 64).
     */
    explicit DemodulatorQAM(int levels_in)
        : levels_count_(levels_in),
          bits_per_symbol_(calculate_bits_per_symbol(levels_in)) {
        if (levels_count_ != 4 && levels_count_ != 16 && levels_count_ != 64) {
            throw std::invalid_argument(
                "DemodulatorQAM: Only 4, 16, and 64 QAM levels are supported");
        }
        generateConstellation();
        generateBitPatterns();
    }

    /**
     * @brief Perform hard decision demodulation of received symbols.
     *
     * Maps each symbol to the closest constellation point and returns
     * corresponding bits.
     *
     * @param symbols Received symbols as pairs of (real, imaginary) values
     * @return Vector of recovered bits
     */
    std::vector<uint8_t> demodulate_hard(
        std::span<const std::pair<value_type, value_type>> symbols) const {
        std::vector<uint8_t> bits;
        if (symbols.empty()) {
            return bits;
        }
        bits.reserve(symbols.size() * bits_per_symbol_);

        for (const auto& s : symbols) {
            int best_idx = 0;
            value_type best_dist_sq =
                std::numeric_limits<value_type>::infinity();

            for (int idx = 0; idx < levels_count_; ++idx) {
                value_type dr = s.first - constellation_[idx].first;
                value_type di = s.second - constellation_[idx].second;
                value_type dist_sq = dr * dr + di * di;
                if (dist_sq < best_dist_sq) {
                    best_dist_sq = dist_sq;
                    best_idx = idx;
                }
            }
            for (int j = 0; j < bits_per_symbol_; ++j) {
                bits.push_back(bit_patterns_[best_idx][j] ? 1 : 0);
            }
        }
        return bits;
    }

    /**
     * @brief Get the constellation diagram used by the demodulator
     */
    const std::vector<std::pair<value_type, value_type>>& getConstellation()
        const {
        return constellation_;
    }

    /**
     * @brief Get the bit patterns associated with each constellation point
     */
    const std::vector<std::vector<bool>>& getBitPatterns() const {
        return bit_patterns_;
    }

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
    }

    void generateBitPatterns() {
        bit_patterns_.clear();
        bit_patterns_.resize(levels_count_,
                             std::vector<bool>(bits_per_symbol_));
        for (int i = 0; i < levels_count_; ++i) {
            for (int j = 0; j < bits_per_symbol_; ++j) {
                bit_patterns_[i][j] = (i >> (bits_per_symbol_ - 1 - j)) & 1;
            }
        }
    }

    const int levels_count_;
    const int bits_per_symbol_;
    std::vector<std::pair<value_type, value_type>> constellation_;
    std::vector<std::vector<bool>> bit_patterns_;
};