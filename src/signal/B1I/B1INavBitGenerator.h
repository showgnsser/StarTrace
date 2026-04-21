/*
******************************************************************************
* @file           : B1INavBitGenerator.h
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : B1I导航电文比特生成器（支持D1/D2电文）
* @attention      : None
* @date           : 2026/4/19
******************************************************************************
*/
#ifndef STARTRACE_B1I_NAV_BIT_GENERATOR_H
#define STARTRACE_B1I_NAV_BIT_GENERATOR_H

#include "NHCode.h"
#include "SignalConfig.h"
#include <cstdint>
#include <vector>

namespace startrace {
namespace signal {

class B1INavBitGenerator {
public:
    // ==================== D1 电文结构常量 ====================
    static constexpr int D1_BITS_PER_WORD = 30;           // 每字比特数
    static constexpr int D1_WORDS_PER_SUBFRAME = 10;      // 每子帧字数
    static constexpr int D1_SUBFRAMES_PER_FRAME = 5;      // 每帧子帧数
    static constexpr int D1_BITS_PER_SUBFRAME = 300;      // 每子帧比特数 (30 * 10)
    static constexpr int D1_BITS_PER_FRAME = 1500;        // 每帧比特数 (300 * 5)
    static constexpr int D1_PRN_PERIODS_PER_BIT = 20;     // 承载每个比特所需的PRN码周期数(1ms*20=20ms)

    // ==================== D2 电文结构常量 ====================
    static constexpr int D2_BITS_PER_WORD = 30;           // 每字比特数
    static constexpr int D2_WORDS_PER_SUBFRAME = 10;      // 每子帧字数
    static constexpr int D2_SUBFRAMES_PER_FRAME = 5;      // 每帧子帧数
    static constexpr int D2_BITS_PER_SUBFRAME = 300;      // 每子帧比特数 (30 * 10)
    static constexpr int D2_BITS_PER_FRAME = 1500;        // 每帧比特数 (300 * 5)
    static constexpr int D2_PRN_PERIODS_PER_BIT = 2;      // 承载每个比特所需的PRN码周期数(1ms*2=2ms)

    /**
     * @brief 构造函数
     * @param prn 卫星PRN号（1-63）
     * @param message_type 电文类型（D1或D2）
     */
    explicit B1INavBitGenerator(int prn, BdsMessageType message_type = BdsMessageType::D1);

    /**
     * @brief 获取指定PRN码周期索引处的调制值 (+1 或 -1)
     * D1: 返回 NH码 × 导航比特
     * D2: 返回 导航比特
     */
    int8_t getModulationValue(int prn_period_index) const;

    /**
     * @brief 设置导航电文比特数据 (自动将0/1转换为+1/-1)
     */
    void setNavBits(const std::vector<uint8_t>& bits);

    /**
     * @brief 生成伪随机导航电文（用于测试）
     * @param num_frames 生成的帧数
     */
    void generateRandomNavData(int num_frames = 1);

    void setMessageType(BdsMessageType message_type, bool regenerate_random = false);

private:
    /**
     * @brief 获取指定PRN码周期索引处的底层导航比特值 (+1 或 -1)
     */
    int8_t getNavBit(int prn_period_index) const;

    int m_prn;                       // 卫星PRN号
    BdsMessageType m_messageType;    // 电文类型

    // 扁平化存储：底层直接存储双极性电平 (+1 或 -1)，避免实时转换
    std::vector<int8_t> m_navBits;
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_B1I_NAV_BIT_GENERATOR_H