/*
******************************************************************************
* @file           : test_nh_code.cpp
* @brief          : NHCode 单元测试
* @date           : 2026/4/19
******************************************************************************
*/
/**
 * @file    test_nh_code.cpp
 * @brief   NHCode 单元测试（适配极简内联重构版）
 */
#include <gtest/gtest.h>
#include "signal/B1I/NHCode.h"

using namespace startrace::signal;

TEST(NHCodeTest, LengthConstant) {
    EXPECT_EQ(NHCode::LENGTH, 20);
}

TEST(NHCodeTest, PreMappedSequence) {
    // 原始序列: {0,0,0,0,0,1,0,0,1,1,0,1,0,1,0,0,1,1,1,0}
    // 重构后已在底层直接映射为: {1,1,1,1,1,-1,1,1,-1,-1,1,-1,1,-1,1,1,-1,-1,-1,1}
    const int8_t expected[20] = {
        1,  1,  1,  1,  1, -1,  1,  1, -1, -1,
        1, -1,  1, -1,  1,  1, -1, -1, -1,  1
    };

    for (int i = 0; i < NHCode::LENGTH; ++i) {
        EXPECT_EQ(NHCode::getChip(i), expected[i]) << "index " << i;
    }
}

TEST(NHCodeTest, IndexWraparound) {
    // 内部自动取模机制测试
    EXPECT_EQ(NHCode::getChip(0), NHCode::getChip(20));
    EXPECT_EQ(NHCode::getChip(5), NHCode::getChip(25));
    EXPECT_EQ(NHCode::getChip(19), NHCode::getChip(39));
    EXPECT_EQ(NHCode::getChip(0), NHCode::getChip(100));
}

TEST(NHCodeTest, GetChipForPrnPeriod) {
    for (int i = 0; i < 40; ++i) {
        EXPECT_EQ(NHCode::getChipForPrnPeriod(i), NHCode::getChip(i));
    }
}