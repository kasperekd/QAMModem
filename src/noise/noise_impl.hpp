#include "qam_simulator/noise.hpp"

template <Numeric T>
std::pmr::vector<std::pair<T, T>> NoiseAdder<T>::addNoiseStandard(
    std::span<const std::pair<T, T>> symbols) const {
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
    std::default_random_engine rng(std::random_device{}());

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
    if constexpr (std::is_same_v<T, float>) {
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

        std::normal_distribution<float> dist(0.0f, sigma);
        std::default_random_engine rng(std::random_device{}());

        std::pmr::vector<std::pair<T, T>> noisy(symbols.size(), alloc_);

        size_t i = 0;
        for (; i + 8 <= symbols.size(); i += 8) {
            __m256 noise_re =
                _mm256_set_ps(dist(rng), dist(rng), dist(rng), dist(rng),
                              dist(rng), dist(rng), dist(rng), dist(rng));
            __m256 noise_im =
                _mm256_set_ps(dist(rng), dist(rng), dist(rng), dist(rng),
                              dist(rng), dist(rng), dist(rng), dist(rng));

            __m256 re = _mm256_loadu_ps(&symbols[i].first);
            __m256 im = _mm256_loadu_ps(&symbols[i].second);

            re = _mm256_add_ps(re, noise_re);
            im = _mm256_add_ps(im, noise_im);

            _mm256_storeu_ps(&noisy[i].first, re);
            _mm256_storeu_ps(&noisy[i].second, im);
        }

        for (; i < symbols.size(); ++i) {
            T noise_re = static_cast<T>(dist(rng));
            T noise_im = static_cast<T>(dist(rng));
            noisy[i] = {symbols[i].first + noise_re,
                        symbols[i].second + noise_im};
        }

        return noisy;
    } else {
        return addNoiseStandard(symbols);  // резерв
    }
}
#endif

template <Numeric T>
std::pmr::vector<std::pair<T, T>> NoiseAdder<T>::addNoise(
    std::span<const std::pair<T, T>> symbols) const {
#ifdef ENABLE_SIMD
    return addNoiseSIMD(symbols);
#else
    return addNoiseStandard(symbols);
#endif
}