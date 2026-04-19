/**
 * @file    test_nav_bit_generator.cpp
 * @brief   B1INavBitGenerator 单元测试（适配重构后代码）
 */
#include <gtest/gtest.h>
#include "signal/B1I/B1INavBitGenerator.h"
#include "signal/B1I/NHCode.h"

namespace startrace {
namespace signal {
namespace test {

class NavBitGeneratorTest : public ::testing::Test {
protected:
    static bool isValidBipolar(int8_t value) {
        return value == 1 || value == -1;
    }
};

// ==================== 构造函数测试 ====================

TEST_F(NavBitGeneratorTest, ConstructorDefaultIsD1) {
    // 新构造：第二参数默认 BdsMessageType::D1
    B1INavBitGenerator gen(6);

    EXPECT_EQ(gen.getPrn(), 6);
    EXPECT_EQ(gen.getMessageType(), BdsMessageType::D1);
}

TEST_F(NavBitGeneratorTest, ConstructorExplicitD1) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    EXPECT_EQ(gen.getMessageType(), BdsMessageType::D1);
    EXPECT_TRUE(gen.usesNHCode());
    EXPECT_EQ(gen.getPrnPeriodsPerBit(), 20u);
}

TEST_F(NavBitGeneratorTest, ConstructorExplicitD2) {
    B1INavBitGenerator gen(1, BdsMessageType::D2);

    EXPECT_EQ(gen.getMessageType(), BdsMessageType::D2);
    EXPECT_FALSE(gen.usesNHCode());
    EXPECT_EQ(gen.getPrnPeriodsPerBit(), 2u);
}

TEST_F(NavBitGeneratorTest, InvalidPrn) {
    EXPECT_THROW(B1INavBitGenerator gen(0),  std::invalid_argument);
    EXPECT_THROW(B1INavBitGenerator gen(64), std::invalid_argument);
    EXPECT_THROW(B1INavBitGenerator gen(0,  BdsMessageType::D2), std::invalid_argument);
    EXPECT_THROW(B1INavBitGenerator gen(99, BdsMessageType::D1), std::invalid_argument);
}

TEST_F(NavBitGeneratorTest, ValidBoundaryPrn) {
    EXPECT_NO_THROW(B1INavBitGenerator gen(1));
    EXPECT_NO_THROW(B1INavBitGenerator gen(63));
}

// ==================== 常量一致性测试 ====================

TEST_F(NavBitGeneratorTest, FrameStructureConstants) {
    // 1500 bit/frame = 5 subframe × 10 word × 30 bit
    EXPECT_EQ(B1INavBitGenerator::BITS_PER_FRAME, 1500u);
    EXPECT_EQ(B1INavBitGenerator::D1_BITS_PER_FRAME, 1500u);
    EXPECT_EQ(B1INavBitGenerator::D2_BITS_PER_FRAME, 1500u);
    EXPECT_EQ(B1INavBitGenerator::D1_PRN_PERIODS_PER_BIT, 20u);
    EXPECT_EQ(B1INavBitGenerator::D2_PRN_PERIODS_PER_BIT, 2u);
}

// ==================== D1 电文调制测试 ====================

TEST_F(NavBitGeneratorTest, D1ModulationWithNHCode) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    // 设置全零比特（bit=0 映射为 +1）
    std::vector<uint8_t> zeroBits(100, 0);
    gen.setNavBits(zeroBits);

    // 第一个比特的 20 个 PRN 周期内，调制值 = NH[i] * (+1) = NH[i]
    for (size_t i = 0; i < NHCode::LENGTH; ++i) {
        EXPECT_EQ(gen.getModulationValue(i), NHCode::getChip(i))
            << "at prn_period_index=" << i;
    }
}

TEST_F(NavBitGeneratorTest, D1ModulationBitFlipWithNHCode) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    // bit=1 -> -1，与 NH 码相乘后整段翻转
    std::vector<uint8_t> oneBits(5, 1);
    gen.setNavBits(oneBits);

    for (size_t i = 0; i < NHCode::LENGTH; ++i) {
        EXPECT_EQ(gen.getModulationValue(i),
                  static_cast<int8_t>(-NHCode::getChip(i)));
    }
}

TEST_F(NavBitGeneratorTest, D1ModulationValidity) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    for (size_t i = 0; i < 200; ++i) {
        EXPECT_TRUE(isValidBipolar(gen.getModulationValue(i)))
            << "Invalid modulation at index " << i;
    }
}

// ==================== D2 电文调制测试 ====================

TEST_F(NavBitGeneratorTest, D2ModulationNoNHCode) {
    B1INavBitGenerator gen(1, BdsMessageType::D2);

    std::vector<uint8_t> testBits = {0, 1, 0, 1};
    gen.setNavBits(testBits);

    // D2: 每 2 个 PRN 周期一个比特，无 NH 码
    for (size_t bitIdx = 0; bitIdx < testBits.size(); ++bitIdx) {
        int8_t expected = (testBits[bitIdx] == 0) ? 1 : -1;
        EXPECT_EQ(gen.getModulationValue(bitIdx * 2),     expected);
        EXPECT_EQ(gen.getModulationValue(bitIdx * 2 + 1), expected);
    }
}

TEST_F(NavBitGeneratorTest, D2ModulationValidity) {
    B1INavBitGenerator gen(1, BdsMessageType::D2);

    for (size_t i = 0; i < 200; ++i) {
        EXPECT_TRUE(isValidBipolar(gen.getModulationValue(i)));
    }
}

// ==================== 导航比特设置与访问 ====================

TEST_F(NavBitGeneratorTest, SetAndGetNavBitsLength) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    std::vector<uint8_t> testBits = {0, 1, 0, 1, 1, 0};
    gen.setNavBits(testBits);

    EXPECT_EQ(gen.getNavBitsLength(), testBits.size());
}

TEST_F(NavBitGeneratorTest, NavBitsWrapAround) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    std::vector<uint8_t> shortBits = {0, 1};
    gen.setNavBits(shortBits);

    // D1: 20 PRN周期/比特，navBits 循环
    // PRN周期 0..19  -> 比特 0
    // PRN周期 20..39 -> 比特 1
    // PRN周期 40..59 -> 比特 0 (循环回)
    EXPECT_EQ(gen.getNavBit(0),  gen.getNavBit(40));
    EXPECT_EQ(gen.getNavBit(20), gen.getNavBit(60));
    EXPECT_NE(gen.getNavBit(0),  gen.getNavBit(20));
}

TEST_F(NavBitGeneratorTest, NavBitBipolarMapping) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    std::vector<uint8_t> bits = {0, 1};
    gen.setNavBits(bits);

    // 约定: 0 -> +1, 1 -> -1
    EXPECT_EQ(gen.getNavBit(0),  1);   // 比特 0 对应 PRN 周期 0..19
    EXPECT_EQ(gen.getNavBit(20), -1);  // 比特 1 对应 PRN 周期 20..39
}

// ==================== 消息类型切换测试 ====================

TEST_F(NavBitGeneratorTest, SetMessageTypeKeepsData) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    std::vector<uint8_t> customBits(100, 0);
    gen.setNavBits(customBits);
    size_t originalLen = gen.getNavBitsLength();

    // 默认 regenerate_random=false，应保留已有数据
    gen.setMessageType(BdsMessageType::D2);
    EXPECT_EQ(gen.getMessageType(), BdsMessageType::D2);
    EXPECT_EQ(gen.getNavBitsLength(), originalLen);
}

TEST_F(NavBitGeneratorTest, SetMessageTypeRegenerates) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    std::vector<uint8_t> customBits(50, 0);
    gen.setNavBits(customBits);

    // 显式要求重新生成随机数据
    gen.setMessageType(BdsMessageType::D2, /*regenerate_random=*/true);
    EXPECT_EQ(gen.getMessageType(), BdsMessageType::D2);
    EXPECT_EQ(gen.getNavBitsLength(), B1INavBitGenerator::BITS_PER_FRAME);
}

TEST_F(NavBitGeneratorTest, SetMessageTypeSameTypeNoChange) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);

    std::vector<uint8_t> customBits(77, 0);
    gen.setNavBits(customBits);

    // 设为相同类型，不应触发任何变化
    gen.setMessageType(BdsMessageType::D1, /*regenerate_random=*/true);
    EXPECT_EQ(gen.getNavBitsLength(), 77u);
}

// ==================== 随机数据生成测试 ====================

TEST_F(NavBitGeneratorTest, GenerateRandomOneFrame) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);
    gen.generateRandomNavData(1);
    EXPECT_EQ(gen.getNavBitsLength(), 1500u);
}

TEST_F(NavBitGeneratorTest, GenerateRandomMultipleFrames) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);
    gen.generateRandomNavData(3);
    EXPECT_EQ(gen.getNavBitsLength(), 1500u * 3);
}

TEST_F(NavBitGeneratorTest, DifferentPrnsGenerateDifferentData) {
    B1INavBitGenerator gen1(6, BdsMessageType::D1);
    B1INavBitGenerator gen2(7, BdsMessageType::D1);

    int diffCount = 0;
    for (size_t i = 0; i < 1500; ++i) {
        if (gen1.getNavBit(i) != gen2.getNavBit(i)) ++diffCount;
    }
    // 不同 PRN 的随机种子不同，应有显著差异（约一半比特不同）
    EXPECT_GT(diffCount, 500);
}

TEST_F(NavBitGeneratorTest, SamePrnSameTypeReproducible) {
    B1INavBitGenerator gen1(6, BdsMessageType::D1);
    B1INavBitGenerator gen2(6, BdsMessageType::D1);

    for (size_t i = 0; i < 1500; ++i) {
        EXPECT_EQ(gen1.getNavBit(i), gen2.getNavBit(i))
            << "Mismatch at bit " << i;
    }
}

// ==================== NH 码本身的测试 ====================

TEST_F(NavBitGeneratorTest, NHCodeLength) {
    EXPECT_EQ(NHCode::LENGTH, 20u);
}

TEST_F(NavBitGeneratorTest, NHCodeBipolar) {
    for (size_t i = 0; i < NHCode::LENGTH; ++i) {
        EXPECT_TRUE(isValidBipolar(NHCode::getChip(i)));
    }
}

TEST_F(NavBitGeneratorTest, NHCodePeriodic) {
    for (size_t i = 0; i < NHCode::LENGTH; ++i) {
        EXPECT_EQ(NHCode::getChip(i), NHCode::getChip(i + NHCode::LENGTH));
        EXPECT_EQ(NHCode::getChip(i), NHCode::getChipForPrnPeriod(i + 40));
    }
}

TEST_F(NavBitGeneratorTest, NHCodeExpectedSequence) {
    // 标准 NH 序列：{0,0,0,0,0,1,0,0,1,1,0,1,0,1,0,0,1,1,1,0}
    // 映射：0 -> +1, 1 -> -1
    const int8_t expected[20] = {
        +1,+1,+1,+1,+1, -1,+1,+1,-1,-1,
        +1,-1,+1,-1,+1, +1,-1,-1,-1,+1
    };
    for (size_t i = 0; i < NHCode::LENGTH; ++i) {
        EXPECT_EQ(NHCode::getChip(i), expected[i]) << "at NH[" << i << "]";
    }
}

} // namespace test
} // namespace signal
} // namespace startrace