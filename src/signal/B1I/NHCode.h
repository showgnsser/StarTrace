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

#include <array>
#include <cstdint>
#include <cstddef>

namespace startrace {
namespace signal {

/**
 * @brief 纽曼-霍夫曼(Neuman-Hoffman)码生成器
 *
 * NH码序列：{0,0,0,0,0,1,0,0,1,1,0,1,0,1,0,0,1,1,1,0}
 * 长度：20位，周期：20ms
 */
class NHCode {
public:
    static constexpr size_t LENGTH = 20;

    static int8_t getChip(size_t index) {
        return (SEQUENCE[index % LENGTH] == 0) ? 1 : -1;
    }

    static int8_t getChipForPrnPeriod(size_t prn_period_index) {
        return getChip(prn_period_index % LENGTH);
    }

private:
    // C++17起 static constexpr 成员隐式inline，无需类外定义
    static constexpr std::array<uint8_t, LENGTH> SEQUENCE = {
        0, 0, 0, 0, 0, 1, 0, 0, 1, 1,
        0, 1, 0, 1, 0, 0, 1, 1, 1, 0
    };
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_NH_CODE_H