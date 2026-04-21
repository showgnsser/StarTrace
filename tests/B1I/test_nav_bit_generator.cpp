/**
 * @file    test_nav_bit_generator.cpp
 * @brief   B1INavBitGenerator 单元测试（适配重构后代码）
 */
#include <gtest/gtest.h>
#include "signal/B1I/B1INavBitGenerator.h"

using namespace startrace::signal;

class NavBitGeneratorTest : public ::testing::Test {
protected:
    static bool isValidBipolar(int8_t value) { return value == 1 || value == -1; }
};

TEST_F(NavBitGeneratorTest, ConstructorExplicitD1) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);
    // PRN 应被正确记录
    // 测试私有成员间接表现
    gen.generateRandomNavData(1);
    EXPECT_TRUE(isValidBipolar(gen.getModulationValue(0)));
}

// 【关键变化】：越界 PRN 回退机制
TEST_F(NavBitGeneratorTest, InvalidPrnFallback) {
    B1INavBitGenerator invalidGen(99, BdsMessageType::D1);
    B1INavBitGenerator fallbackGen(1, BdsMessageType::D1);

    // 强制使用相同种子（因为PRN相同）重置随机数据
    invalidGen.generateRandomNavData(1);
    fallbackGen.generateRandomNavData(1);

    // 应该生成相同的随机比特序列
    int matchCount = 0;
    for (int i = 0; i < 1500; ++i) {
        if (invalidGen.getModulationValue(i * 20) == fallbackGen.getModulationValue(i * 20)) {
            matchCount++;
        }
    }
    EXPECT_EQ(matchCount, 1500); // 全部一致
}

TEST_F(NavBitGeneratorTest, FrameStructureConstants) {
    EXPECT_EQ(B1INavBitGenerator::D1_BITS_PER_FRAME, 1500);
    EXPECT_EQ(B1INavBitGenerator::D2_BITS_PER_FRAME, 1500);
    EXPECT_EQ(B1INavBitGenerator::D1_PRN_PERIODS_PER_BIT, 20);
    EXPECT_EQ(B1INavBitGenerator::D2_PRN_PERIODS_PER_BIT, 2);
}

TEST_F(NavBitGeneratorTest, SetNavBitsMapping) {
    B1INavBitGenerator gen(6, BdsMessageType::D1);
    std::vector<uint8_t> bits = {0, 1}; // 0->+1, 1->-1
    gen.setNavBits(bits);

    // 对于 D1 而言，前 20 个 PRN 周期由 第一个 bit(+1) 控制
    // 但受到 NH 码调制
    EXPECT_EQ(gen.getModulationValue(0), NHCode::getChip(0) * 1);

    // 后 20 个 PRN 周期由 第二个 bit(-1) 控制
    EXPECT_EQ(gen.getModulationValue(20), NHCode::getChip(20) * (-1));
}

TEST_F(NavBitGeneratorTest, D2ModulationNoNHCode) {
    B1INavBitGenerator gen(1, BdsMessageType::D2);
    std::vector<uint8_t> testBits = {0, 1};
    gen.setNavBits(testBits);

    // D2 每 2 个 PRN 周期一个比特，且无 NH 码
    EXPECT_EQ(gen.getModulationValue(0),  1);
    EXPECT_EQ(gen.getModulationValue(1),  1);
    EXPECT_EQ(gen.getModulationValue(2), -1);
    EXPECT_EQ(gen.getModulationValue(3), -1);
}