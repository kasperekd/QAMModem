#pragma once

#include <cmath>
#include <complex>
#include <concepts>
#include <memory_resource>
#include <span>
#include <stdexcept>
#include <vector>

#ifndef _NUMERIC
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;
#define _NUMERIC
#endif  // !_NUMERIC

/**
 * @brief Template class for QAM (Quadrature Amplitude Modulation) demodulator.
 *
 * This class supports hard and soft demodulation for QPSK (4-QAM), 16-QAM, and
 * 64-QAM. It maps received complex symbols back to bit sequences using
 * constellation matching.
 *
 * @tparam Levels Number of constellation points (must be 4, 16, or 64)
 * @tparam T      Numeric type used for representing constellation coordinates
 */
template <int Levels, Numeric T>
class DemodulatorQAM {
   public:
    /// @brief Number of bits encoded in each symbol
    static constexpr int BitsPerSymbol = std::log2(Levels);

    /// @brief Number of constellation points
    static constexpr int LevelsCount = Levels;

    /// @brief Type used for numeric representation of constellation points
    using value_type = T;

    /**
     * @brief Construct a new DemodulatorQAM object
     *
     * @param resource Memory resource for memory allocation
     */
    explicit DemodulatorQAM(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : alloc_(resource) {
        static_assert(Levels == 4 || Levels == 16 || Levels == 64,
                      "Only 4, 16, and 64 QAM are supported");
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
    std::pmr::vector<uint8_t> demodulate_hard(
        std::span<const std::pair<T, T>> symbols) const;

    /**
     * @brief Perform soft decision demodulation of received symbols.
     *
     * Computes log-likelihood ratios (LLRs) for each bit based on Euclidean
     * distances.
     *
     * @param symbols Received symbols as pairs of (real, imaginary) values
     * @param sigma Standard deviation of AWGN noise
     * @return Vector of LLR values for each bit
     */
    std::pmr::vector<float> demodulate_soft(
        std::span<const std::pair<T, T>> symbols, float sigma) const {
        std::pmr::vector<float> llrs(alloc_);
        float sigma_sq = sigma * sigma;

        for (const auto& s : symbols) {
            for (int j = 0; j < BitsPerSymbol; ++j) {
                float min_dist0 = INFINITY;
                float min_dist1 = INFINITY;
                for (int idx = 0; idx < Levels; ++idx) {
                    bool bit = bit_patterns_[idx][j];
                    float dist = distance_squared(s, constellation_[idx]);
                    if (bit) {
                        if (dist < min_dist1) min_dist1 = dist;
                    } else {
                        if (dist < min_dist0) min_dist0 = dist;
                    }
                }
                float llr = (min_dist0 - min_dist1) / (2.0f * sigma_sq);
                llrs.push_back(llr);
            }
        }

        return llrs;
    }

    /**
     * @brief Compute squared Euclidean distance between two points
     *
     * @param a First point (real, imaginary)
     * @param b Second point (real, imaginary)
     * @return Squared distance
     */
    float distance_squared(const std::pair<T, T>& a,
                           const std::pair<T, T>& b) const {
        float dr = static_cast<float>(a.first) - static_cast<float>(b.first);
        float di = static_cast<float>(a.second) - static_cast<float>(b.second);
        return dr * dr + di * di;
    }

    /**
     * @brief Get the constellation diagram used by the demodulator
     *
     * @return Const reference to the constellation vector
     */
    const std::pmr::vector<std::pair<T, T>>& getConstellation() const {
        return constellation_;
    }

    /**
     * @brief Get the bit patterns associated with each constellation point
     *
     * @return Const reference to the bit pattern vector
     */
    const std::pmr::vector<std::pmr::vector<bool>>& getBitPatterns() const {
        return bit_patterns_;
    }

   private:
    /**
     * @brief Generate constellation diagram based on modulation scheme
     */
    void generateConstellation();

    /**
     * @brief Generate bit patterns for each constellation point
     */
    void generateBitPatterns();

    /**
     * @brief Convert Gray-coded index to binary
     *
     * @param g Gray code value
     * @return Binary equivalent
     */
    static int from_gray(int g) {
        int b = 0;
        for (; g; g >>= 1) {
            b ^= g;
        }
        return b;
    }

    std::pmr::polymorphic_allocator<T> alloc_;
    std::pmr::vector<std::pair<T, T>> constellation_;
    std::pmr::vector<std::pmr::vector<bool>> bit_patterns_;
};

// --- Specializations for constellation generation ---
template <int Levels, Numeric T>
void DemodulatorQAM<Levels, T>::generateConstellation() {
    constellation_.clear();
    if constexpr (Levels == 4) {  // QPSK with natural Gray coding
        constellation_ = {{+1, +1}, {-1, +1}, {-1, -1}, {+1, -1}};
    } else if constexpr (Levels == 16) {  // 16QAM with Gray coding
        const int pam_levels[] = {-3, -1, 1, 3};
        for (int idx = 0; idx < Levels; ++idx) {
            int upper_bits = (idx >> 2) & 0x03;
            int lower_bits = idx & 0x03;
            int gray_upper = upper_bits ^ (upper_bits >> 1);
            int gray_lower = lower_bits ^ (lower_bits >> 1);
            T re = static_cast<T>(pam_levels[gray_upper]);
            T im = static_cast<T>(pam_levels[gray_lower]);
            constellation_.emplace_back(re, im);
        }
    } else if constexpr (Levels == 64) {  // 64QAM with Gray coding
        const int pam_levels[] = {-7, -5, -3, -1, 1, 3, 5, 7};
        for (int idx = 0; idx < Levels; ++idx) {
            int gray_j = (idx >> 3) & 0x07;
            int gray_i = idx & 0x07;
            int j = from_gray(gray_j);
            int i = from_gray(gray_i);
            T re = static_cast<T>(pam_levels[j]);
            T im = static_cast<T>(pam_levels[i]);
            constellation_.emplace_back(re, im);
        }
    }
}

// --- Bit pattern generation ---
template <int Levels, Numeric T>
void DemodulatorQAM<Levels, T>::generateBitPatterns() {
    bit_patterns_.clear();
    bit_patterns_.reserve(Levels);
    for (int i = 0; i < Levels; ++i) {
        std::pmr::vector<bool> pattern(alloc_);
        pattern.resize(BitsPerSymbol);
        for (int j = 0; j < BitsPerSymbol; ++j) {
            int bit_pos = BitsPerSymbol - 1 - j;
            pattern[j] = (i >> bit_pos) & 1;
        }
        bit_patterns_.push_back(pattern);
    }
}

template <int Levels, Numeric T>
std::pmr::vector<uint8_t> DemodulatorQAM<Levels, T>::demodulate_hard(
    std::span<const std::pair<T, T>> symbols) const {
    std::pmr::vector<uint8_t> bits(alloc_);
    for (const auto& s : symbols) {
        int best_idx = 0;
        float best_dist = distance_squared(s, constellation_[0]);
        for (size_t idx = 1; idx < Levels; ++idx) {
            float dist = distance_squared(s, constellation_[idx]);
            if (dist < best_dist) {
                best_dist = dist;
                best_idx = idx;
            }
        }
        for (int j = 0; j < BitsPerSymbol; ++j) {
            bits.push_back(bit_patterns_[best_idx][j] ? 1 : 0);
        }
    }
    return bits;
}