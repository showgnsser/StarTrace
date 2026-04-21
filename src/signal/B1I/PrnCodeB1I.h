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

namespace startrace {
namespace signal {

class PrnCodeB1I {
public:
    // ==================== B1I测距码物理常量 ====================
    // B1I测距码的固定长度（单位：码片）
    static constexpr int CODE_LENGTH = 2046;
    // 线性反馈移位寄存器 (LFSR) 的级数 (G1和G2均为11级)
    static constexpr int SHIFT_REGISTER_STAGES = 11;
    // 北斗卫星合法PRN编号的最小值
    static constexpr int MIN_PRN = 1;
    // 北斗卫星合法PRN编号的最大值
    static constexpr int MAX_PRN = 63;

    // ==================== 算法结构常量（消除魔鬼数字） ====================
    // PRN查找表大小 (考虑到数组从0开始索引，大小需为 MAX_PRN + 1)
    static constexpr int PRN_TABLE_SIZE = MAX_PRN + 1;
    // G2相位选择的最大抽头数量（北斗B1I最多允许3个抽头异或）
    static constexpr int MAX_G2_PHASE_TAPS = 3;
    // G1反馈多项式的有效抽头数
    static constexpr int G1_TAP_COUNT = 6;
    // G2反馈多项式的有效抽头数
    static constexpr int G2_TAP_COUNT = 8;

    /**
     * @brief 生成指定 PRN 编号的 B1I 测距码
     * @param prn_number 卫星 PRN 编号 (1 ~ 63)
     * @return std::vector<int8_t> 包含 2046 个码片的序列，值为 +1 或 -1
     */
    static std::vector<int8_t> generate(int prn_number);

private:
    /**
     * @brief 执行单次线性反馈移位寄存器 (LFSR) 移位操作
     * @param reg 移位寄存器状态数组
     * @param feedback_taps 反馈抽头索引数组（以0结尾）
     * @param max_taps 最大可能抽头数（用于循环控制）
     */
    static void shiftLfsr(uint8_t* reg, const int* feedback_taps, int max_taps);
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_PRN_CODE_B1I_H