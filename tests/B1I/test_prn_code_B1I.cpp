// test_prn_code.cpp
#include <gtest/gtest.h>
#include "signal/B1I/PrnCodeB1I.h"
#include <cmath>

using namespace startrace::signal;

class PrnCodeB1ITest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试PRN码长度
TEST_F(PrnCodeB1ITest, CodeLength) {
    auto code = PrnCodeB1I::generate(1);
    EXPECT_EQ(code.size(), PrnCodeB1I::CODE_LENGTH);
    EXPECT_EQ(code.size(), 2046);
}

// 测试有效PRN范围
TEST_F(PrnCodeB1ITest, ValidPrnRange) {
    EXPECT_NO_THROW(PrnCodeB1I::generate(1));
    EXPECT_NO_THROW(PrnCodeB1I::generate(10));
}

// 测试无效PRN
TEST_F(PrnCodeB1ITest, InvalidPrn) {
    EXPECT_THROW(PrnCodeB1I::generate(0), std::invalid_argument);
    EXPECT_THROW(PrnCodeB1I::generate(-1), std::invalid_argument);
}

// 测试码值范围（只能是+1或-1）
TEST_F(PrnCodeB1ITest, ChipValueRange) {
    auto code = PrnCodeB1I::generate(1);
    for (size_t i = 0; i < code.size(); ++i) {
        EXPECT_TRUE(code[i] == 1 || code[i] == -1)
            << "Chip at index " << i << " is " << static_cast<int>(code[i]);
    }
}

// 测试不同PRN产生不同码序列
TEST_F(PrnCodeB1ITest, DifferentPrnDifferentCode) {
    auto code1 = PrnCodeB1I::generate(1);
    auto code2 = PrnCodeB1I::generate(2);

    bool all_same = true;
    for (size_t i = 0; i < code1.size(); ++i) {
        if (code1[i] != code2[i]) {
            all_same = false;
            break;
        }
    }
    EXPECT_FALSE(all_same);
}

// 测试相同PRN产生相同码序列
TEST_F(PrnCodeB1ITest, SamePrnSameCode) {
    auto code1 = PrnCodeB1I::generate(10);
    auto code2 = PrnCodeB1I::generate(10);

    EXPECT_EQ(code1.size(), code2.size());
    for (size_t i = 0; i < code1.size(); ++i) {
        EXPECT_EQ(code1[i], code2[i]);
    }
}

// 测试码平衡性（+1和-1数量应接近）
TEST_F(PrnCodeB1ITest, CodeBalance) {
    auto code = PrnCodeB1I::generate(1);

    int sum = 0;
    for (const auto& chip : code) {
        sum += chip;
    }

    // 对于Gold码，+1和-1的数量差应该很小（通常差1）
    EXPECT_LE(std::abs(sum), 10);
}

// 测试常量定义
TEST_F(PrnCodeB1ITest, Constants) {
    EXPECT_EQ(PrnCodeB1I::CODE_LENGTH, 2046);
    EXPECT_EQ(PrnCodeB1I::SHIFT_REGISTER_STAGES, 11);
    EXPECT_EQ(PrnCodeB1I::MIN_PRN, 1);
    EXPECT_EQ(PrnCodeB1I::MAX_PRN, 63);
}

// 测试初始状态
TEST_F(PrnCodeB1ITest, InitialState) {
    const auto& state = PrnCodeB1I::INITIAL_STATE;
    EXPECT_EQ(state.size(), PrnCodeB1I::SHIFT_REGISTER_STAGES);
}

// 测试多个PRN的码序列都有效
TEST_F(PrnCodeB1ITest, MultiplePrnValid) {
    // 测试已定义的PRN（根据你的表，1-10应该已定义）
    for (int prn = 1; prn <= 10; ++prn) {
        auto code = PrnCodeB1I::generate(prn);
        EXPECT_EQ(code.size(), PrnCodeB1I::CODE_LENGTH) << "PRN " << prn;

        for (size_t i = 0; i < code.size(); ++i) {
            EXPECT_TRUE(code[i] == 1 || code[i] == -1)
                << "PRN " << prn << " chip at index " << i;
        }
    }
}