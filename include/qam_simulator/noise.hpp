#pragma once

#include <cmath>
#include <random>
#include <span>
#include <utility>
#include <vector>

/**
 * @brief Class for adding AWGN noise to a signal.
 *
 * This class provides functionality to add Additive White Gaussian Noise (AWGN)
 * to complex-valued symbols. The noise characteristics are determined by the
 * Signal-to-Noise Ratio (SNR).
 */
class NoiseAdder {
   public:
    using value_type = float;

    /**
     * @brief Constructor that initializes the noise generator with a given SNR.
     *
     * @param snr_db Signal-to-Noise Ratio in decibels (dB). This SNR is used to
     *               calculate noise variance relative to the input signal
     * power.
     */
    explicit NoiseAdder(double snr_db)
        : snr_db_(snr_db), rng_(std::mt19937(std::random_device{}())) {}

    /**
     * @brief Adds AWGN noise to a sequence of input symbols.
     *
     * The method calculates the power of the input signal, then determines the
     * required noise variance based on the specified SNR. Gaussian noise is
     * generated and added to each symbol.
     *
     * @param symbols A span of constant complex symbols (pairs of real and
     * imaginary values) to which noise will be added.
     * @return A vector of complex symbols with added noise. Returns an empty
     * vector if the input span is empty.
     */
    std::vector<std::pair<value_type, value_type>> addNoise(
        std::span<const std::pair<value_type, value_type>> symbols) const {
        if (symbols.empty()) {
            return {};
        }

        double signal_power = 0.0;
        for (const auto& symbol : symbols) {
            double re = static_cast<double>(symbol.first);
            double im = static_cast<double>(symbol.second);
            signal_power += re * re + im * im;
        }
        signal_power /= static_cast<double>(symbols.size());

        if (signal_power < 1e-9) {
        }

        double snr_linear = std::pow(10.0, snr_db_ / 10.0);
        double noise_power = (snr_linear == 0)
                                 ? std::numeric_limits<double>::infinity()
                                 : (signal_power / snr_linear);
        double sigma_component_double = std::sqrt(noise_power / 2.0);
        value_type sigma_component =
            static_cast<value_type>(sigma_component_double);

        std::normal_distribution<value_type> dist(0.0f, sigma_component);

        std::vector<std::pair<value_type, value_type>> noisy_symbols;
        noisy_symbols.reserve(symbols.size());

        for (const auto& symbol : symbols) {
            value_type noise_re = dist(rng_);
            value_type noise_im = dist(rng_);
            noisy_symbols.emplace_back(symbol.first + noise_re,
                                       symbol.second + noise_im);
        }

        return noisy_symbols;
    }

    /**
     * @brief Get the current SNR value in dB.
     *
     * @return Current SNR level in decibels.
     */
    double getSNRdb() const { return snr_db_; }

   private:
    double snr_db_;
    mutable std::mt19937 rng_;
};