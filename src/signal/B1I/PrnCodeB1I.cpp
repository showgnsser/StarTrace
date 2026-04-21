/*
******************************************************************************
* @file           : prn_code_b1i.cpp
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : None
* @attention      : None
* @date           : 2026/4/13
******************************************************************************
*/
#include "PrnCodeB1I.h"


#include "PrnCodeB1I.h"

namespace startrace {
namespace signal {

// ==================== 生成器内部静态常量 ====================

// 固定的初始状态：所有北斗B1I测距码生成器的G1和G2寄存器初始填充值
static const uint8_t INITIAL_STATE[PrnCodeB1I::SHIFT_REGISTER_STAGES] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};

// G1生成多项式的反馈抽头位置（数组以 0 作为结束符标志）
static const int G1_FEEDBACK_TAPS[] = {1, 7, 8, 9, 10, 11, 0};

// G2生成多项式的反馈抽头位置（数组以 0 作为结束符标志）
static const int G2_FEEDBACK_TAPS[] = {1, 2, 3, 4, 5, 8, 9, 11, 0};

// G2寄存器平移等效相位选择表：不同PRN号对应不同的G2抽头组合进行异或
// 数组维度 [PRN_TABLE_SIZE][MAX_G2_PHASE_TAPS]
static const int G2_PHASE_SEL_TABLE[PrnCodeB1I::PRN_TABLE_SIZE][PrnCodeB1I::MAX_G2_PHASE_TAPS] = {
    {0, 0, 0},       // Index 0: 预留未使用（使得数组索引直接等于PRN号）
    {1, 3, 0},       // PRN 1
    {1, 4, 0},       // PRN 2
    {1, 5, 0},       // PRN 3
    {1, 6, 0},       // PRN 4
    {1, 8, 0},       // PRN 5
    {1, 9, 0},       // PRN 6
    {1, 10, 0},      // PRN 7
    {1, 11, 0},      // PRN 8
    {2, 7, 0},       // PRN 9
    {3, 4, 0},       // PRN 10
    {3, 5, 0},       // PRN 11
    {3, 6, 0},       // PRN 12
    {3, 8, 0},       // PRN 13
    {3, 9, 0},       // PRN 14
    {3, 10, 0},      // PRN 15
    {3, 11, 0},      // PRN 16
    {4, 5, 0},       // PRN 17
    {4, 6, 0},       // PRN 18
    {4, 8, 0},       // PRN 19
    {4, 9, 0},       // PRN 20
    {4, 10, 0},      // PRN 21
    {4, 11, 0},      // PRN 22
    {5, 6, 0},       // PRN 23
    {5, 8, 0},       // PRN 24
    {5, 9, 0},       // PRN 25
    {5, 10, 0},      // PRN 26
    {5, 11, 0},      // PRN 27
    {6, 8, 0},       // PRN 28
    {6, 9, 0},       // PRN 29
    {6, 10, 0},      // PRN 30
    {6, 11, 0},      // PRN 31
    {8, 9, 0},       // PRN 32
    {8, 10, 0},      // PRN 33
    {8, 11, 0},      // PRN 34
    {9, 10, 0},      // PRN 35
    {9, 11, 0},      // PRN 36
    {10, 11, 0},     // PRN 37
    {1, 2, 7},       // PRN 38  (3个抽头)
    {1, 3, 4},       // PRN 39
    {1, 3, 6},       // PRN 40
    {1, 3, 8},       // PRN 41
    {1, 3, 10},      // PRN 42
    {1, 3, 11},      // PRN 43
    {1, 4, 5},       // PRN 44
    {1, 4, 9},       // PRN 45
    {1, 5, 6},       // PRN 46
    {1, 5, 8},       // PRN 47
    {1, 5, 10},      // PRN 48
    {1, 5, 11},      // PRN 49
    {1, 6, 9},       // PRN 50
    {1, 8, 9},       // PRN 51
    {1, 9, 10},      // PRN 52
    {1, 9, 11},      // PRN 53
    {2, 3, 7},       // PRN 54
    {2, 5, 7},       // PRN 55
    {2, 7, 9},       // PRN 56
    {3, 4, 5},       // PRN 57
    {3, 4, 9},       // PRN 58
    {3, 5, 6},       // PRN 59
    {3, 5, 8},       // PRN 60
    {3, 5, 10},      // PRN 61
    {3, 5, 11},      // PRN 62
    {3, 6, 9}        // PRN 63
};

void PrnCodeB1I::shiftLfsr(uint8_t* reg, const int* feedback_taps, int max_taps) {
    uint8_t feedback = 0;
    for (int i = 0; i < max_taps; ++i) {
        if (feedback_taps[i] == 0) break;
        feedback ^= reg[feedback_taps[i] - 1];
    }
    for (int i = SHIFT_REGISTER_STAGES - 1; i > 0; --i) {
        reg[i] = reg[i - 1];
    }
    reg[0] = feedback;
}

std::vector<int8_t> PrnCodeB1I::generate(int prn_number) {
    if (prn_number < MIN_PRN || prn_number > MAX_PRN) {
        prn_number = 1;
    }

    std::vector<int8_t> prn_sequence;
    prn_sequence.reserve(CODE_LENGTH);

    const int* g2_taps = G2_PHASE_SEL_TABLE[prn_number];

    uint8_t g1_reg[SHIFT_REGISTER_STAGES];
    uint8_t g2_reg[SHIFT_REGISTER_STAGES];

    for (int i = 0; i < SHIFT_REGISTER_STAGES; ++i) {
        g1_reg[i] = INITIAL_STATE[i];
        g2_reg[i] = INITIAL_STATE[i];
    }

    for (int i = 0; i < CODE_LENGTH; ++i) {
        uint8_t g1_out = g1_reg[SHIFT_REGISTER_STAGES - 1];

        uint8_t g2_out = 0;
        // 消除魔鬼数字 3
        for (int tap_idx = 0; tap_idx < MAX_G2_PHASE_TAPS; ++tap_idx) {
            int tap = g2_taps[tap_idx];
            if (tap == 0) break;
            g2_out ^= g2_reg[tap - 1];
        }

        uint8_t chip_bit = g1_out ^ g2_out;
        prn_sequence.push_back((chip_bit == 0) ? 1 : -1);

        // 消除魔鬼数字 6 和 8
        shiftLfsr(g1_reg, G1_FEEDBACK_TAPS, G1_TAP_COUNT);
        shiftLfsr(g2_reg, G2_FEEDBACK_TAPS, G2_TAP_COUNT);
    }

    return prn_sequence;
}

} // namespace signal
} // namespace startrace