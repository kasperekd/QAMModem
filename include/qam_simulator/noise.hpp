#pragma once

#include <cmath>
#include <memory_resource>
#include <random>
#include <span>
#include <utility>
#include <vector>

#ifndef _NUMERIC
/**
 * @brief Concept for numeric types (arithmetic types)
 *
 * This concept checks if a type is an arithmetic type (e.g., int, float,
 * double).
 */
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;
#define _NUMERIC
#endif  // !_NUMERIC

/**
 * @brief Class for adding AWGN noise to a signal with optional SIMD
 * optimization.
 *
 * This class provides functionality to add Additive White Gaussian Noise (AWGN)
 * to complex-valued symbols. It supports both standard and optionally
 * SIMD-accelerated implementations depending on build configuration.
 *
 * @tparam T Data type used for representing symbols (e.g., float or double)
 */
template <Numeric T>
class NoiseAdder {
   public:
    /**
     * @brief Constructor that initializes the noise generator with a given SNR.
     *
     * @param snr_db Signal-to-Noise Ratio in decibels (dB)
     * @param resource Pointer to memory resource for allocation
     */
    explicit NoiseAdder(double snr_db, std::pmr::memory_resource* resource =
                                           std::pmr::get_default_resource())
        : snr_db_(snr_db), alloc_(resource) {}

    /**
     * @brief Main method to add noise to input symbols.
     *
     * Depending on compile-time settings, this method may use either the
     * standard or SIMD-optimized implementation.
     *
     * @param symbols Input symbol span as pairs of (real, imaginary)
     * @return Vector of noisy symbols as pairs of (real, imaginary)
     */
    std::pmr::vector<std::pair<T, T>> addNoise(
        std::span<const std::pair<T, T>> symbols) const;

    /**
     * @brief Get the current SNR value in dB
     *
     * @return Current SNR level in decibels
     */
    T getSNRdb() const { return snr_db_; }

   private:
    double snr_db_;
    std::pmr::polymorphic_allocator<T> alloc_;

    /**
     * @brief Standard implementation of noise addition without SIMD
     * acceleration.
     *
     * Uses C++ standard library random number generation to apply AWGN.
     *
     * @param symbols Input symbols as pair span
     * @return Noisy output symbols
     */
    std::pmr::vector<std::pair<T, T>> addNoiseStandard(
        std::span<const std::pair<T, T>> symbols) const;

#ifdef ENABLE_SIMD
    /**
     * @brief SIMD-optimized implementation of noise addition using AVX2
     * instructions.
     *
     * This function is only available when `ENABLE_SIMD` is defined.
     *
     * @param symbols Input symbols as pair span
     * @return Noisy output symbols
     */
    std::pmr::vector<std::pair<T, T>> addNoiseSIMD(
        std::span<const std::pair<T, T>> symbols) const;
#endif
};

#include "src/noise/noise_impl.hpp"