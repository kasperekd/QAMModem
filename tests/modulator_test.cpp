#include <gtest/gtest.h>

#include <cmath>
#include <memory_resource>
#include <span>
#include <vector>

#include "qam_simulator/modulator_qam.hpp"

/**
 * @brief Define specific modulator types for testing.
 *
 * These instantiations cover different modulation orders and numeric types.
 */
using QPSKFloat = ModulatorQAM<4, float>;
using QAM16Float = ModulatorQAM<16, float>;
using QAM64Float = ModulatorQAM<64, float>;
using QPSKInt16 = ModulatorQAM<4, int16_t>;
using QAM16Int32 = ModulatorQAM<16, int32_t>;

using ModulatorTypes =
    ::testing::Types<QPSKFloat, QAM16Float, QAM64Float, QPSKInt16, QAM16Int32>;

/**
 * @brief Test fixture for generic modulator tests.
 *
 * Provides memory resources for polymorphic allocation used in the modulator.
 */
template <typename ModulatorType>
class ModulatorTest : public ::testing::Test {
   protected:
    alignas(alignof(std::max_align_t)) mutable std::array<char, 4096> buffer;
    std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
    std::pmr::polymorphic_allocator<typename ModulatorType::value_type> alloc{
        &mbr};
};

TYPED_TEST_SUITE(ModulatorTest, ModulatorTypes);

/**
 * @brief Tests that symbol count is computed correctly.
 *
 * Checks that:
 * - Empty input returns empty output.
 * - Input bit size divisible by BitsPerSymbol yields correct number of symbols.
 */
TYPED_TEST(ModulatorTest, ConstellationSize) {
    using Modulator = TypeParam;
    constexpr int BitsPerSymbol = Modulator::BitsPerSymbol;

    EXPECT_TRUE(BitsPerSymbol == 2 || BitsPerSymbol == 4 || BitsPerSymbol == 6)
        << "Invalid BitsPerSymbol: " << BitsPerSymbol;

    Modulator modulator(1.0f, &this->mbr);

    auto empty_span = std::span<const uint8_t>{};
    auto empty_symbols = modulator.modulate(empty_span);
    EXPECT_EQ(empty_symbols.size(), 0);

    if (BitsPerSymbol == 2 || BitsPerSymbol == 4) {
        std::vector<uint8_t> bits(1024, 0);
        auto bit_span = std::span<const uint8_t>(bits.data(), bits.size());
        auto symbols = modulator.modulate(bit_span);

        EXPECT_EQ(bit_span.size(), 1024) << "Bit span size mismatch";
        EXPECT_EQ(symbols.size(), bits.size() / BitsPerSymbol)
            << "Symbol size mismatch for BitsPerSymbol=" << BitsPerSymbol;
    }

    if (BitsPerSymbol == 6) {
        std::vector<uint8_t> bits(1020, 0);
        auto bit_span = std::span<const uint8_t>(bits.data(), bits.size());
        auto symbols = modulator.modulate(bit_span);

        EXPECT_EQ(bit_span.size(), 1020) << "Bit span size mismatch for 64QAM";
        EXPECT_EQ(symbols.size(), bits.size() / BitsPerSymbol)
            << "Symbol size mismatch for BitsPerSymbol=6";
    }
}

/**
 * @brief Verifies constellation mapping for QPSK (4-QAM).
 *
 * The symbol for bit pattern [0,0] should be (+1, +1).
 */
TEST(QPSKConstellationTest, CorrectSymbols) {
    ModulatorQAM<4, float> modulator(1.0f);
    std::array<uint8_t, 2> bits = {0, 0};
    auto symbols =
        modulator.modulate(std::span<const uint8_t>(bits.data(), bits.size()));
    EXPECT_FLOAT_EQ(symbols[0].first, 1.0f);
    EXPECT_FLOAT_EQ(symbols[0].second, 1.0f);
}

/**
 * @brief Verifies constellation mapping for 16-QAM.
 *
 * The symbol for bit pattern [0,0,0,0] should be (-1, -1) when scaled by 1/3.
 */
TEST(SixteenQAMConstellationTest, CorrectSymbols) {
    ModulatorQAM<16, float> modulator(1.0f / 3.0f);
    std::array<uint8_t, 4> bits = {0, 0, 0, 0};
    auto symbols =
        modulator.modulate(std::span<const uint8_t>(bits.data(), bits.size()));
    EXPECT_FLOAT_EQ(symbols[0].first, -1.0f);
    EXPECT_FLOAT_EQ(symbols[0].second, -1.0f);
}

/**
 * @brief Tests scaling behavior with fixed-point arithmetic (int16_t).
 *
 * Ensures that the scale factor is applied correctly to integer outputs.
 */
TEST(FixedPointScalingTest, CorrectScaling) {
    ModulatorQAM<4, int16_t> modulator(16384);  // scale_factor ≈ 1/4096
    std::array<uint8_t, 2> bits = {0, 0};
    auto symbols =
        modulator.modulate(std::span<const uint8_t>(bits.data(), bits.size()));
    EXPECT_EQ(symbols[0].first, 16384);
    EXPECT_EQ(symbols[0].second, 16384);
}

/**
 * @brief Tests validation of input bit count.
 *
 * Modulation must fail if bit count is not divisible by BitsPerSymbol.
 */
TEST(BitCountValidationTest, InvalidBitCount) {
    ModulatorQAM<4, float> qpsk;
    ModulatorQAM<16, float> qam16;
    ModulatorQAM<64, float> qam64;

    std::array<uint8_t, 1> bits1 = {0};
    std::array<uint8_t, 3> bits3 = {0, 0, 0};
    std::array<uint8_t, 5> bits5 = {0, 0, 0, 0, 0};

    EXPECT_THROW(
        qpsk.modulate(std::span<const uint8_t>(bits1.data(), bits1.size())),
        std::invalid_argument);
    EXPECT_THROW(
        qam16.modulate(std::span<const uint8_t>(bits3.data(), bits3.size())),
        std::invalid_argument);
    EXPECT_THROW(
        qam64.modulate(std::span<const uint8_t>(bits5.data(), bits5.size())),
        std::invalid_argument);
}