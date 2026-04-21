/**
 * @file    test_prn_code_B1I.cpp
 * @brief   PrnCodeB1I 单元测试（适配防御性降级版）
 */
#include <gtest/gtest.h>
#include "signal/B1I/PrnCodeB1I.h"
#include <cmath>

using namespace startrace::signal;

class PrnCodeB1ITest : public ::testing::Test {};

TEST_F(PrnCodeB1ITest, CodeLength) {
    auto code = PrnCodeB1I::generate(1);
    EXPECT_EQ(code.size(), PrnCodeB1I::CODE_LENGTH);
}

TEST_F(PrnCodeB1ITest, ValidPrnRange) {
    auto code1 = PrnCodeB1I::generate(1);
    auto code63 = PrnCodeB1I::generate(63);
    EXPECT_EQ(code1.size(), 2046);
    EXPECT_EQ(code63.size(), 2046);
}

// 【关键变化】：越界不再抛出异常，而是降级为 PRN 1
TEST_F(PrnCodeB1ITest, InvalidPrnFallback) {
    auto fallbackCode0  = PrnCodeB1I::generate(0);
    auto fallbackCode99 = PrnCodeB1I::generate(99);
    auto validCode1     = PrnCodeB1I::generate(1);

    EXPECT_EQ(fallbackCode0, validCode1);
    EXPECT_EQ(fallbackCode99, validCode1);
}

TEST_F(PrnCodeB1ITest, DifferentPrnDifferentCode) {
    auto code1 = PrnCodeB1I::generate(1);
    auto code2 = PrnCodeB1I::generate(2);

    bool any_diff = false;
    for (size_t i = 0; i < code1.size(); ++i) {
        if (code1[i] != code2[i]) {
            any_diff = true;
            break;
        }
    }
    EXPECT_TRUE(any_diff);
}

TEST_F(PrnCodeB1ITest, CodeBalance) {
    auto code = PrnCodeB1I::generate(1);
    int sum = 0;
    for (int8_t chip : code) { sum += chip; }
    // Gold码特性：+1和-1的数量差应极小
    EXPECT_LE(std::abs(sum), 10);
}

TEST_F(PrnCodeB1ITest, Constants) {
    EXPECT_EQ(PrnCodeB1I::CODE_LENGTH, 2046);
    EXPECT_EQ(PrnCodeB1I::SHIFT_REGISTER_STAGES, 11);
    EXPECT_EQ(PrnCodeB1I::MAX_PRN, 63);
    EXPECT_EQ(PrnCodeB1I::PRN_TABLE_SIZE, 64);
    EXPECT_EQ(PrnCodeB1I::MAX_G2_PHASE_TAPS, 3);
}