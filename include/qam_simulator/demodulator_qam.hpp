#pragma once

#include <cmath>
#include <complex>
#include <concepts>
#include <memory_resource>
#include <stdexcept>
#include <vector>

template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

template <int Levels, Numeric T>
class DemodulatorQAM {
   public:
    static constexpr int BitsPerSymbol = std::log2(Levels);

    using value_type = T;

    explicit DemodulatorQAM(
        T scale_factor = 1.0,
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : alloc_(resource), scale_factor_(scale_factor) {
        static_assert(Levels == 4 || Levels == 16 || Levels == 64,
                      "Only 4, 16, and 64 QAM are supported");
        generateConstellation();
        generateBitPatterns();
    }

    std::pmr::vector<uint8_t> demodulate_hard(
        const std::vector<std::complex<T>>& symbols) const {
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

    std::pmr::vector<float> demodulate_soft(
        const std::vector<std::complex<T>>& symbols, float sigma = 1.0f) const {
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

   private:
    void generateConstellation();
    void generateBitPatterns();

    float distance_squared(const std::complex<T>& a,
                           const std::complex<T>& b) const {
        float dr = static_cast<float>(a.real()) - static_cast<float>(b.real());
        float di = static_cast<float>(a.imag()) - static_cast<float>(b.imag());
        return dr * dr + di * di;
    }

    static int from_gray(int g) {
        int b = 0;
        for (; g; g >>= 1) {
            b ^= g;
        }
        return b;
    }

    std::pmr::polymorphic_allocator<T> alloc_;
    std::pmr::vector<std::complex<T>> constellation_;
    std::pmr::vector<std::pmr::vector<bool>> bit_patterns_;
    T scale_factor_;
};

template <int Levels, Numeric T>
void DemodulatorQAM<Levels, T>::generateConstellation() {
    constellation_.clear();
    if constexpr (Levels == 4) {
        constellation_ = {std::complex<T>(1, 1), std::complex<T>(-1, 1),
                          std::complex<T>(-1, -1), std::complex<T>(1, -1)};
    } else if constexpr (Levels == 16) {
        const int pam_levels[] = {-3, -1, 1, 3};
        for (int idx = 0; idx < Levels; ++idx) {
            int upper_bits = (idx >> 2) & 0x03;
            int lower_bits = idx & 0x03;
            int gray_upper = upper_bits ^ (upper_bits >> 1);
            int gray_lower = lower_bits ^ (lower_bits >> 1);
            T re = static_cast<T>(pam_levels[gray_upper]) * scale_factor_;
            T im = static_cast<T>(pam_levels[gray_lower]) * scale_factor_;
            constellation_.emplace_back(re, im);
        }
    } else if constexpr (Levels == 64) {
        const int pam_levels[] = {-7, -5, -3, -1, 1, 3, 5, 7};
        for (int idx = 0; idx < Levels; ++idx) {
            int gray_j = (idx >> 3) & 0x07;
            int gray_i = idx & 0x07;
            int j = from_gray(gray_j);
            int i = from_gray(gray_i);
            T re = static_cast<T>(pam_levels[j]) * scale_factor_;
            T im = static_cast<T>(pam_levels[i]) * scale_factor_;
            constellation_.emplace_back(re, im);
        }
    }
}

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