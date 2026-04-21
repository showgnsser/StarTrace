/*
******************************************************************************
* @file           : NHCode.h
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : None
* @attention      : None
* @date           : 2026/4/19
******************************************************************************
*/
#ifndef STARTRACE_NH_CODE_H
#define STARTRACE_NH_CODE_H

#include <cstdint>

namespace startrace {
namespace signal {

/**
 * @brief 纽曼-霍夫曼(Neuman-Hoffman)码生成器
 * * NH码用于北斗D1电文的二次调制。
 * 原始序列：{0,0,0,0,0,1,0,0,1,1,0,1,0,1,0,0,1,1,1,0}
 * 为提升实时算力，已在底层直接映射为双极性电平：0 -> +1, 1 -> -1
 */
class NHCode {
public:
    // ==================== NH码物理常量 ====================
    // NH码的固定长度（单位：位，即20ms周期）
    static constexpr int LENGTH = 20;

    /**
     * @brief 获取指定索引的NH码片值（双极性电平）
     * @param index 索引（无需手动防越界，内部会自动取模）
     * @return int8_t 码片值 (+1 或 -1)
     */
    static inline int8_t getChip(int index) {
        // 静态预映射数组，消灭运行时的条件分支，极致的查表速度
        static const int8_t SEQUENCE[LENGTH] = {
            1,  1,  1,  1,  1, -1,  1,  1, -1, -1,
            1, -1,  1, -1,  1,  1, -1, -1, -1,  1
       };
        return SEQUENCE[index % LENGTH];
    }

    /**
     * @brief 根据PRN周期索引获取当前对应的NH码片
     */
    static inline int8_t getChipForPrnPeriod(int prn_period_index) {
        return getChip(prn_period_index);
    }
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_NH_CODE_H