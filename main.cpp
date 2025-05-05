#include <iomanip>
#include <iostream>
#include <numeric>

#include "qam_simulator/demodulator_qam.hpp"
#include "qam_simulator/modulator_qam.hpp"
#include "qam_simulator/noise.hpp"

std::vector<uint8_t> generateRandomBits(size_t num_bits) {
    std::vector<uint8_t> bits(num_bits);
    for (size_t i = 0; i < num_bits; ++i) {
        bits[i] = static_cast<uint8_t>(rand() % 2);
    }
    return bits;
}

template <typename Vector>
void printBits(const std::string& label, const Vector& bits,
               size_t max_bits = 20) {
    std::cout << label << ": ";
    for (size_t i = 0; i < std::min(max_bits, bits.size()); ++i) {
        std::cout << static_cast<int>(bits[i]);
        if (i < max_bits - 1 && i < bits.size() - 1) std::cout << " ";
    }
    if (bits.size() > max_bits) {
        std::cout << " ... (" << bits.size() << " bits total)";
    }
    std::cout << std::endl;
}

int main() {
    using Modulator = ModulatorQAM<16, float>;
    using Demodulator = DemodulatorQAM<16, float>;
    using NoiseAdder = NoiseAdder<float>;

    size_t num_bits = 1073741824;

    std::vector<uint8_t> bits = generateRandomBits(num_bits);

    constexpr int BitsPerSymbol = 4;
    if (bits.size() % BitsPerSymbol != 0) {
        std::cerr << "Error: Bit count must be divisible by " << BitsPerSymbol
                  << std::endl;
        return 1;
    }
    std::cout << "=== QAM Simulation ===" << std::endl;

    // Модуляция
    Modulator modulator;
    auto symbols = modulator.modulate(bits);
    std::cout << "Modulated symbols: " << symbols.size() << std::endl;

    // Добавление шума
    NoiseAdder noise_adder(8.0);
    auto noisy_symbols = noise_adder.addNoise(symbols);
    std::cout << "Added AWGN noise" << std::endl;

    // Демодуляция
    Demodulator demodulator;
    auto recovered_bits = demodulator.demodulate_hard(noisy_symbols);
    std::cout << "Demodulated bits: " << recovered_bits.size() << std::endl;

    printBits("Original", bits);
    printBits("Recovered", recovered_bits);

    if (bits.size() != recovered_bits.size()) {
        std::cerr << "ERROR: Bit count mismatch!" << std::endl;
        std::cout << "Expected: " << bits.size()
                  << " bits, Got: " << recovered_bits.size() << " bits"
                  << std::endl;
        return 1;
    }

    // ошибоки
    size_t errors = 0;
    for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i] != recovered_bits[i]) errors++;
    }

    std::cout << "\nBit Error Rate (BER): " << std::fixed
              << std::setprecision(2) << (errors * 100.0) / bits.size() << "% ("
              << errors << " / " << bits.size() << " bits)" << std::endl;

    std::cout << (errors == 0 ? "All bits recovered correctly!"
                              : "Bit errors detected")
              << std::endl;

    return 0;
}