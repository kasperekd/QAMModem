#include <gtest/gtest.h>

#include "qam_simulator/demodulator_qam.hpp"

using DemodulatorQPSKFloat = DemodulatorQAM<4, float>;
using DemodulatorQAM16Float = DemodulatorQAM<16, float>;
using DemodulatorQAM64Float = DemodulatorQAM<64, float>;
using DemodulatorQPSKInt16 = DemodulatorQAM<4, int16_t>;
using DemodulatorQAM16Int32 = DemodulatorQAM<16, int32_t>;

using DemodulatorTypes =
    ::testing::Types<DemodulatorQPSKFloat, DemodulatorQAM16Float,
                     DemodulatorQAM64Float, DemodulatorQPSKInt16,
                     DemodulatorQAM16Int32>;

template <typename DemodulatorType>
class DemodulatorTest : public ::testing::Test {
   protected:
    alignas(alignof(std::max_align_t)) mutable std::array<char, 4096> buffer;
    std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
    std::pmr::polymorphic_allocator<typename DemodulatorType::value_type> alloc{
        &mbr};
};

TYPED_TEST_SUITE(DemodulatorTest, DemodulatorTypes);

TYPED_TEST(DemodulatorTest, ConstellationMapping) {
    using Demodulator = TypeParam;
    Demodulator demodulator(&this->mbr);

    std::vector<std::pair<typename Demodulator::value_type,
                          typename Demodulator::value_type>>
        symbols;

    if constexpr (Demodulator::BitsPerSymbol == 2) {  // QPSK
        symbols = {{+1, +1}, {-1, +1}, {-1, -1}, {+1, -1}};
    } else if constexpr (Demodulator::BitsPerSymbol == 4) {  // 16-QAM
        symbols = {{-3, -3}, {-3, -1}, {-3, 3}, {-3, 1}, {-1, -3}, {-1, -1},
                   {-1, 3},  {-1, 1},  {3, -3}, {3, -1}, {3, 3},   {3, 1},
                   {1, -3},  {1, -1},  {1, 3},  {1, 1}};
    } else if constexpr (Demodulator::BitsPerSymbol == 6) {  // 64-QAM
        symbols = {{-7, -7}, {-7, -5}, {-7, -1}, {-7, -3}, {-7, 7},  {-7, 5},
                   {-7, 1},  {-7, 3},  {-5, -7}, {-5, -5}, {-5, -1}, {-5, -3},
                   {-5, 7},  {-5, 5},  {-5, 1},  {-5, 3},  {-1, -7}, {-1, -5},
                   {-1, -1}, {-1, -3}, {-1, 7},  {-1, 5},  {-1, 1},  {-1, 3},
                   {-3, -7}, {-3, -5}, {-3, -1}, {-3, -3}, {-3, 7},  {-3, 5},
                   {-3, 1},  {-3, 3},  {7, -7},  {7, -5},  {7, -1},  {7, -3},
                   {7, 7},   {7, 5},   {7, 1},   {7, 3},   {5, -7},  {5, -5},
                   {5, -1},  {5, -3},  {5, 7},   {5, 5},   {5, 1},   {5, 3},
                   {1, -7},  {1, -5},  {1, -1},  {1, -3},  {1, 7},   {1, 5},
                   {1, 1},   {1, 3},   {3, -7},  {3, -5},  {3, -1},  {3, -3},
                   {3, 7},   {3, 5},   {3, 1},   {3, 3}};
    }

    for (size_t i = 0; i < symbols.size(); ++i) {
        auto bits = demodulator.demodulate_hard({&symbols[i], 1});

        int best_idx = -1;
        float best_dist = std::numeric_limits<float>::infinity();

        const auto& constellation = demodulator.getConstellation();
        for (int idx = 0; idx < Demodulator::LevelsCount; ++idx) {
            float dist =
                demodulator.distance_squared(symbols[i], constellation[idx]);
            if (dist < best_dist) {
                best_dist = dist;
                best_idx = idx;
            }
        }

        const auto& bit_patterns = demodulator.getBitPatterns();

        std::cout << "Test case " << i << ":\n"
                  << "  Input symbol: (" << symbols[i].first << ", "
                  << symbols[i].second << ")\n"
                  << "  Best index: " << best_idx << "\n"
                  << "  Expected bits: ";
        for (int j = 0; j < Demodulator::BitsPerSymbol; ++j) {
            int bit_pos = Demodulator::BitsPerSymbol - 1 - j;
            std::cout << ((i >> bit_pos) & 1);
        }
        std::cout << "\n  Actual bits:   ";
        for (const auto& bit : bits) {
            std::cout << bit;
        }
        std::cout << "\n  Bit pattern for index " << best_idx << ": ";
        for (const auto& bit : bit_patterns[best_idx]) {
            std::cout << bit;
        }
        std::cout << "\n";

        ASSERT_EQ(bits.size(), Demodulator::BitsPerSymbol);
        for (int j = 0; j < Demodulator::BitsPerSymbol; ++j) {
            int bit_pos = Demodulator::BitsPerSymbol - 1 - j;
            uint8_t expected_bit = (i >> bit_pos) & 1;
            EXPECT_EQ(bits[j], expected_bit)
                << "Mismatch at bit " << j << " for symbol index " << i
                << " (best_idx = " << best_idx << ")"
                << " (expected bit: " << expected_bit
                << ", actual bit: " << bits[j] << ")";
        }
    }
}