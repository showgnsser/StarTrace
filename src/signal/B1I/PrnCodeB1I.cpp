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


namespace startrace::signal {

const std::vector<uint8_t> PrnCodeB1I::INITIAL_STATE = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};

static const std::vector<int> G1_FEEDBACK_TAPS = {1, 7, 8, 9, 10, 11};
static const std::vector<int> G2_FEEDBACK_TAPS = {1, 2, 3, 4, 5, 8, 9, 11};

// 静态查找表：存储所有 PRN 的 G2 相位选择抽头
// 索引 0 留空，使得 PRN 编号可以直接作为索引 (例如 PRN 1 对应索引 1)
static const std::vector<std::vector<int>> G2_PHASE_SEL_TABLE = {
    {},              // Index 0 (Unused)
    {1, 3},          // PRN 1
    {1, 4},          // PRN 2
    {1, 5},          // PRN 3
    {1, 6},          // PRN 4
    {1, 8},          // PRN 5
    {1, 9},          // PRN 6
    {1, 10},         // PRN 7
    {1, 11},         // PRN 8
    {2, 7},          // PRN 9
    {3, 4},          // PRN 10
    {3, 5},          // PRN 11
    {3, 6},          // PRN 12
    {3, 8},          // PRN 13
    {3, 9},          // PRN 14
    {3, 10},         // PRN 15
    {3, 11},         // PRN 16
    {4, 5},          // PRN 17
    {4, 6},          // PRN 18
    {4, 8},          // PRN 19
    {4, 9},          // PRN 20
    {4, 10},         // PRN 21
    {4, 11},         // PRN 22
    {5, 6},          // PRN 23
    {5, 8},          // PRN 24
    {5, 9},          // PRN 25
    {5, 10},         // PRN 26
    {5, 11},         // PRN 27
    {6, 8},          // PRN 28
    {6, 9},          // PRN 29
    {6, 10},         // PRN 30
    {6, 11},         // PRN 31
    {8, 9},          // PRN 32
    {8, 10},         // PRN 33
    {8, 11},         // PRN 34
    {9, 10},         // PRN 35
    {9, 11},         // PRN 36
    {10, 11},        // PRN 37
    {1, 2, 7},       // PRN 38
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
    {3, 6, 9},       // PRN 63
};

const std::vector<int>& PrnCodeB1I::getG2PhaseTaps(int prn_number) {
    if (prn_number < MIN_PRN || prn_number > MAX_PRN) {
        throw std::invalid_argument("PRN number out of range for BDS B1I.");
    }
    // 检查表是否被完整填充（防止越界访问未补全的表）
    if (prn_number >= G2_PHASE_SEL_TABLE.size() || G2_PHASE_SEL_TABLE[prn_number].empty()) {
        throw std::out_of_range("Phase taps for this PRN are not defined in the lookup table yet.");
    }
    return G2_PHASE_SEL_TABLE[prn_number];
}

void PrnCodeB1I::ShiftLfsr(std::vector<uint8_t>& reg, const std::vector<int>& feedback_taps) {
    uint8_t feedback = 0;
    for (int tap : feedback_taps) {
        feedback ^= reg[tap - 1];
    }
    for (size_t i = SHIFT_REGISTER_STAGES - 1; i > 0; --i) {
        reg[i] = reg[i - 1];
    }
    reg[0] = feedback;
}

std::vector<int8_t> PrnCodeB1I::generate(int prn_number) {
    std::vector<int8_t> prn_sequence;
    prn_sequence.reserve(CODE_LENGTH);

    // 获取当前 PRN 对应的 G2 相位选择抽头 (可能是2个或3个)
    const std::vector<int>& g2_taps = getG2PhaseTaps(prn_number);

    std::vector<uint8_t> g1_reg = INITIAL_STATE;
    std::vector<uint8_t> g2_reg = INITIAL_STATE;

    for (size_t i = 0; i < CODE_LENGTH; ++i) {
        uint8_t g1_out = g1_reg[SHIFT_REGISTER_STAGES - 1];

        // 动态计算 G2 输出：遍历所有配置的抽头进行异或
        uint8_t g2_out = 0;
        for (const int tap : g2_taps) {
            g2_out ^= g2_reg[tap - 1];
        }

        uint8_t chip_bit = g1_out ^ g2_out;
        int8_t mapped_chip = (chip_bit == 0) ? 1 : -1;
        prn_sequence.push_back(mapped_chip);

        ShiftLfsr(g1_reg, G1_FEEDBACK_TAPS);
        ShiftLfsr(g2_reg, G2_FEEDBACK_TAPS);
    }

    return prn_sequence;
}

} // namespace startrace::signal
