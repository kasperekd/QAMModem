#include "qam_simulator/noise.hpp"

template <Numeric T>
std::pmr::vector<std::pair<T, T>> NoiseAdder<T>::addNoiseStandard(
    std::span<const std::pair<T, T>> symbols) const {
    /**
     * @brief Standard implementation of noise addition without SIMD
     * optimization.
     *
     * This function computes the signal power from input symbols, calculates
     * the noise power based on the SNR, and adds Gaussian noise using the
     * standard C++ random number generator.
     *
     * @param symbols Input symbols as a span of (real, imaginary) pairs.
     * @return Noisy symbols as a vector of (real, imaginary) pairs.
     */
    if (symbols.empty()) return {};

    double signal_power = 0.0;
    for (const auto& [re, im] : symbols) {
        signal_power +=
            static_cast<double>(re) * re + static_cast<double>(im) * im;
    }
    signal_power /= symbols.size();

    double snr_linear = std::pow(10.0, snr_db_ / 10.0);
    double noise_power = signal_power / snr_linear;
    double sigma = std::sqrt(noise_power / 2.0);

    std::normal_distribution<T> dist(0.0, static_cast<T>(sigma));
    std::default_random_engine rng(std::mt19937{}());

    std::pmr::vector<std::pair<T, T>> noisy(symbols.size(), alloc_);
    for (size_t i = 0; i < symbols.size(); ++i) {
        T noise_re = dist(rng);
        T noise_im = dist(rng);
        noisy[i] = {symbols[i].first + noise_re, symbols[i].second + noise_im};
    }

    return noisy;
}

// SIMD
#ifdef ENABLE_SIMD
#include <immintrin.h>

template <Numeric T>
std::pmr::vector<std::pair<T, T>> NoiseAdder<T>::addNoiseSIMD(
    std::span<const std::pair<T, T>> symbols) const {
    /**
     * @brief SIMD-optimized implementation of noise addition using AVX2
     * instructions.
     *
     * This function uses AVX2 intrinsics to process multiple samples in
     * parallel, significantly improving performance for large symbol sets when
     * enabled.
     *
     * @param symbols Input symbols as a span of (real, imaginary) pairs.
     * @return Noisy symbols as a vector of (real, imaginary) pairs.
     */
    if constexpr (std::is_same_v<T, float>) {
        if (symbols.empty()) return {};

        const size_t N = symbols.size();

        // === Calculate signal power ===
        double signal_power = 0.0;
        for (const auto& [re, im] : symbols) {
            signal_power +=
                static_cast<double>(re) * re + static_cast<double>(im) * im;
        }
        signal_power /= N;

        double snr_linear = std::pow(10.0, snr_db_ / 10.0);
        double noise_power = signal_power / snr_linear;
        float sigma = static_cast<float>(std::sqrt(noise_power / 2.0));

        // === Prepare distribution ===
        std::normal_distribution<float> dist(0.0f, sigma);
        std::default_random_engine rng(std::random_device{}());

        // === SoA: separate arrays for real and imaginary parts ===
        std::pmr::vector<float> real(N, alloc_);
        std::pmr::vector<float> imag(N, alloc_);

        for (size_t i = 0; i < N; ++i) {
            real[i] = symbols[i].first;
            imag[i] = symbols[i].second;
        }

        // === SIMD noise addition ===
        size_t i = 0;
        for (; i + 8 <= N; i += 8) {
            alignas(32) float noise_re[8];
            alignas(32) float noise_im[8];

            for (int j = 0; j < 8; ++j) {
                noise_re[j] = dist(rng);
                noise_im[j] = dist(rng);
            }

            __m256 re = _mm256_loadu_ps(&real[i]);
            __m256 im = _mm256_loadu_ps(&imag[i]);

            __m256 nre = _mm256_load_ps(noise_re);
            __m256 nim = _mm256_load_ps(noise_im);

            re = _mm256_add_ps(re, nre);
            im = _mm256_add_ps(im, nim);

            _mm256_storeu_ps(&real[i], re);
            _mm256_storeu_ps(&imag[i], im);
        }

        // Remaining elements
        for (; i < N; ++i) {
            real[i] += dist(rng);
            imag[i] += dist(rng);
        }

        // === Combine back into pairs ===
        std::pmr::vector<std::pair<T, T>> noisy(N, alloc_);
        for (size_t j = 0; j < N; ++j) {
            noisy[j] = {real[j], imag[j]};
        }

        return noisy;
    } else {
        return addNoiseStandard(symbols);
    }
}
#endif

template <Numeric T>
std::pmr::vector<std::pair<T, T>> NoiseAdder<T>::addNoise(
    std::span<const std::pair<T, T>> symbols) const {
    /**
     * @brief Main method to add AWGN noise to input symbols.
     *
     * This method selects between the standard or SIMD-optimized implementation
     * based on the build configuration (`ENABLE_SIMD`).
     *
     * @param symbols Input symbol span as pairs of (real, imaginary)
     * @return Vector of noisy symbols as pairs of (real, imaginary)
     */
#ifdef ENABLE_SIMD
    return addNoiseSIMD(symbols);
#else
    return addNoiseStandard(symbols);
#endif
}