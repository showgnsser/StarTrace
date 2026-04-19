/*
******************************************************************************
* @file           : test_nh_code.cpp
* @brief          : NHCode 单元测试
* @date           : 2026/4/19
******************************************************************************
*/
#include <gtest/gtest.h>
#include "signal/B1I/NHCode.h"

using namespace startrace::signal;

// 测试NH码长度
TEST(NHCodeTest, Length) {
    EXPECT_EQ(NHCode::LENGTH, 20u);
}

// 测试完整序列（0映射为+1，1映射为-1）
TEST(NHCodeTest, FullSequence) {
    // 原始序列: {0,0,0,0,0,1,0,0,1,1,0,1,0,1,0,0,1,1,1,0}
    // 映射后:   {1,1,1,1,1,-1,1,1,-1,-1,1,-1,1,-1,1,1,-1,-1,-1,1}
    std::array<int8_t, 20> expected = {
        1, 1, 1, 1, 1, -1, 1, 1, -1, -1,
        1, -1, 1, -1, 1, 1, -1, -1, -1, 1
    };

    for (size_t i = 0; i < NHCode::LENGTH; ++i) {
        EXPECT_EQ(NHCode::getChip(i), expected[i]) << "index " << i;
    }
}

// 测试索引自动取模
TEST(NHCodeTest, IndexWraparound) {
    EXPECT_EQ(NHCode::getChip(0), NHCode::getChip(20));
    EXPECT_EQ(NHCode::getChip(5), NHCode::getChip(25));
    EXPECT_EQ(NHCode::getChip(19), NHCode::getChip(39));
    EXPECT_EQ(NHCode::getChip(0), NHCode::getChip(100));
}

// 测试getChipForPrnPeriod
TEST(NHCodeTest, GetChipForPrnPeriod) {
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_EQ(NHCode::getChipForPrnPeriod(i), NHCode::getChip(i));
        EXPECT_EQ(NHCode::getChipForPrnPeriod(i + 20), NHCode::getChip(i));
    }
}

// 测试码值只有+1或-1
TEST(NHCodeTest, ChipValueRange) {
    for (size_t i = 0; i < NHCode::LENGTH; ++i) {
        int8_t chip = NHCode::getChip(i);
        EXPECT_TRUE(chip == 1 || chip == -1);
    }
}