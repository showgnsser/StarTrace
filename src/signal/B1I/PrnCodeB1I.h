/*
******************************************************************************
* @file           : prn_code_b1i.h
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : None
* @attention      : None
* @date           : 2026/4/13
******************************************************************************
*/

#ifndef STARTRACE_PRN_CODE_B1I_H
#define STARTRACE_PRN_CODE_B1I_H

#include <vector>
#include <cstdint>
#include <stdexcept>

namespace startrace {
namespace signal {

class PrnCodeB1I {
public:
    static constexpr size_t CODE_LENGTH = 2046;
    static constexpr size_t SHIFT_REGISTER_STAGES = 11;
    static constexpr int MIN_PRN = 1;
    static constexpr int MAX_PRN = 63; // 北斗 B1I 最大支持到 PRN 63

    static const std::vector<uint8_t> INITIAL_STATE;

    /**
     * @brief 生成指定 PRN 编号的 B1I 测距码
     *
     * @param prn_number 卫星 PRN 编号 (1 ~ 63)
     * @return std::vector<int8_t> 包含 2046 个码片的序列，值为 +1 或 -1
     */
    static std::vector<int8_t> generate(int prn_number);

private:
    /**
     * @brief 获取指定 PRN 的 G2 寄存器相位选择抽头
     * @return std::vector<int> 抽头索引列表 (可能包含 2 个或 3 个抽头)
     */
    static const std::vector<int>& getG2PhaseTaps(int prn_number);

    /**
     * @brief 执行单次线性反馈移位寄存器 (LFSR) 移位操作
     */
    static void ShiftLfsr(std::vector<uint8_t>& reg, const std::vector<int>& feedback_taps);
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_PRN_CODE_B1I_H
