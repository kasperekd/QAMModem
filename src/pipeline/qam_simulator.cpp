#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "qam_simulator/csv_writer.hpp"
#include "qam_simulator/demodulator_qam.hpp"
#include "qam_simulator/modulator_qam.hpp"
#include "qam_simulator/noise.hpp"
#include "qam_simulator/pipeline.hpp"

/**
 * @brief Generates a vector of random bits.
 */
std::vector<uint8_t> generateRandomBits(size_t n, std::mt19937& rng) {
    std::vector<uint8_t> v(n);
    std::uniform_int_distribution<int> d(0, 1);
    for (size_t i = 0; i < n; ++i) v[i] = static_cast<uint8_t>(d(rng));
    return v;
}

/**
 * @brief Parses command-line arguments into a SimulationParams structure.
 */
SimulationParams parse_args(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "Usage: " << argv[0]
                  << " <snr_start> <snr_end> <snr_step> <num_threads> "
                     "<bits_per_thread> <iterations_per_snr>\n";
        std::exit(EXIT_FAILURE);
    }
    return {
        std::stod(argv[1]),    // snr_start
        std::stod(argv[2]),    // snr_end
        std::stod(argv[3]),    // snr_step
        std::stoi(argv[4]),    // num_threads
        std::stoull(argv[5]),  // bits_per_thread
        std::stoull(argv[6])   // iterations_per_snr
    };
}

/**
 * @brief Simulates a QAM modulation system for a given M-ary constellation.
 *
 * @param modulation_levels Number of constellation points (e.g., 4 for QPSK, 16
 * for 16-QAM, etc.)
 * @param name Name of the modulation scheme for output file naming.
 * @param p Simulation parameters.
 */
void simulate_mod(int modulation_levels, const std::string& name,
                  const SimulationParams& p) {
    CsvWriter writer("ber_" + name + ".csv");
    writer.write_header("SNR_dB,BER");
    std::cout << "=== " << name << " ===\n";

    std::vector<double> snrs;
    for (double snr = p.snr_start; snr <= p.snr_end; snr += p.snr_step)
        snrs.push_back(snr);

    std::vector<std::atomic<uint64_t>> errors(snrs.size());
    std::vector<std::atomic<uint64_t>> bits(snrs.size());

    for (size_t i = 0; i < snrs.size(); ++i) {
        errors[i] = 0;
        bits[i] = 0;
    }

    std::vector<std::thread> threads;
    threads.reserve(p.num_threads);
    ModulatorQAM mod(modulation_levels);
    DemodulatorQAM demod(modulation_levels);

    for (int t_idx = 0; t_idx < p.num_threads; ++t_idx) {
        unsigned seed =
            static_cast<unsigned>(std::chrono::high_resolution_clock::now()
                                      .time_since_epoch()
                                      .count()) +
            t_idx;

        threads.emplace_back([&, seed]() {
            std::mt19937 rng(seed);
            for (size_t i = 0; i < snrs.size(); ++i) {
                NoiseAdder noise(snrs[i]);

                for (size_t iter = 0; iter < p.iterations_per_snr; ++iter) {
                    auto b = generateRandomBits(p.bits_per_thread, rng);
                    auto s = mod.modulate(b);
                    auto n = noise.addNoise(s);
                    auto r = demod.demodulate_hard(n);

                    uint64_t err = 0;
                    for (size_t j = 0; j < b.size(); ++j)
                        if (b[j] != r[j]) ++err;

                    errors[i] += err;
                    bits[i] += b.size();
                }
            }
        });
    }

    for (auto& th : threads) th.join();

    for (size_t i = 0; i < snrs.size(); ++i) {
        double ber =
            static_cast<double>(errors[i]) / static_cast<double>(bits[i]);
        writer.write_row(snrs[i], ber);
        std::cout << "SNR=" << std::fixed << std::setprecision(12) << snrs[i]
                  << " dB, BER=" << ber << ", Errors=" << errors[i]
                  << ", Bits=" << bits[i] << "\n";
    }
    std::cout << std::defaultfloat;
}

/**
 * @brief Runs all simulations for different QAM modulation schemes.
 */
void run_all_simulations(const SimulationParams& p) {
    auto adjust_bits = [&](size_t bits_per_symbol) {
        size_t rem = p.bits_per_thread % bits_per_symbol;
        return rem == 0 ? p.bits_per_thread
                        : p.bits_per_thread + (bits_per_symbol - rem);
    };

    // QPSK
    {
        SimulationParams p2 = p;
        p2.bits_per_thread = adjust_bits(2);
        if (p2.bits_per_thread != p.bits_per_thread)
            std::cout << "Note: bits_per_thread padded from "
                      << p.bits_per_thread << " to " << p2.bits_per_thread
                      << " for QPSK\n";
        simulate_mod(4, "qpsk", p2);
    }
    // 16-QAM
    {
        SimulationParams p4 = p;
        p4.bits_per_thread = adjust_bits(4);
        if (p4.bits_per_thread != p.bits_per_thread)
            std::cout << "Note: bits_per_thread padded from "
                      << p.bits_per_thread << " to " << p4.bits_per_thread
                      << " for 16-QAM\n";
        simulate_mod(16, "qam16", p4);
    }
    // 64-QAM
    {
        SimulationParams p6 = p;
        p6.bits_per_thread = adjust_bits(6);
        if (p6.bits_per_thread != p.bits_per_thread)
            std::cout << "Note: bits_per_thread padded from "
                      << p.bits_per_thread << " to " << p6.bits_per_thread
                      << " for 64-QAM\n";
        simulate_mod(64, "qam64", p6);
    }
}