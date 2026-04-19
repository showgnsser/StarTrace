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
#include <cstddef>
#include <vector>

namespace startrace {
namespace signal {

/**
 * @brief B1I信号导航电文比特生成器（支持D1和D2电文）
 *
 * D1电文（MEO/IGSO卫星）：
 * - 帧长度：1500比特，持续30秒
 * - 比特速率：50bps
 * - 每个导航比特持续20ms，由20个PRN码周期承载
 * - 使用NH码进行二次调制
 *
 * D2电文（GEO卫星）：
 * - 帧长度：1500比特，持续3秒
 * - 比特速率：500bps
 * - 每个导航比特持续2ms，由2个PRN码周期承载
 * - 不使用NH码
 *
 * 信号调制关系：
 * D1: 最终信号 = PRN码 × NH码 × 导航比特
 * D2: 最终信号 = PRN码 × 导航比特
 */
class B1INavBitGenerator {
public:
    // D1电文结构常量
    static constexpr size_t D1_BITS_PER_WORD = 30;
    static constexpr size_t D1_WORDS_PER_SUBFRAME = 10;
    static constexpr size_t D1_SUBFRAMES_PER_FRAME = 5;
    static constexpr size_t D1_BITS_PER_SUBFRAME = D1_BITS_PER_WORD * D1_WORDS_PER_SUBFRAME;  // 300
    static constexpr size_t D1_BITS_PER_FRAME = D1_BITS_PER_SUBFRAME * D1_SUBFRAMES_PER_FRAME; // 1500
    static constexpr size_t D1_PRN_PERIODS_PER_BIT = 20;
    static constexpr double D1_BIT_DURATION_SEC = 0.020;        // 20ms per bit

    // D2电文结构常量
    static constexpr size_t D2_BITS_PER_WORD = 30;
    static constexpr size_t D2_WORDS_PER_SUBFRAME = 10;
    static constexpr size_t D2_SUBFRAMES_PER_FRAME = 5;
    static constexpr size_t D2_BITS_PER_SUBFRAME = D2_BITS_PER_WORD * D2_WORDS_PER_SUBFRAME;  // 300
    static constexpr size_t D2_BITS_PER_FRAME = D2_BITS_PER_SUBFRAME * D2_SUBFRAMES_PER_FRAME; // 1500
    static constexpr size_t D2_PRN_PERIODS_PER_BIT = 2;
    static constexpr double D2_BIT_DURATION_SEC = 0.002;        // 2ms per bit

    // 通用常量（保持向后兼容）
    static constexpr size_t BITS_PER_WORD = 30;
    static constexpr size_t WORDS_PER_SUBFRAME = 10;
    static constexpr size_t SUBFRAMES_PER_FRAME = 5;
    static constexpr size_t BITS_PER_SUBFRAME = BITS_PER_WORD * WORDS_PER_SUBFRAME;
    static constexpr size_t BITS_PER_FRAME = BITS_PER_SUBFRAME * SUBFRAMES_PER_FRAME;

    /**
     * @brief 构造函数
     * @param prn 卫星PRN号（1-63）
     * @param message_type 电文类型（D1或D2）
     */
    explicit B1INavBitGenerator(uint8_t prn, BdsMessageType message_type = BdsMessageType::D1);

    /**
     * @brief 获取指定PRN码周期索引处的调制值
     *
     * D1电文：返回值 = NH码值 × 导航比特值
     * D2电文：返回值 = 导航比特值（无NH码）
     *
     * @param prn_period_index 从仿真开始的PRN码周期索引（每个周期1ms）
     * @return int8_t 调制值 (+1 或 -1)
     */
    int8_t getModulationValue(size_t prn_period_index) const;

    /**
     * @brief 获取指定PRN码周期索引处的导航比特值（不含NH调制）
     *
     * @param prn_period_index PRN码周期索引
     * @return int8_t 导航比特值 (+1 或 -1)
     */
    int8_t getNavBit(size_t prn_period_index) const;

    /**
     * @brief 设置导航电文比特数据
     *
     * @param bits 导航比特序列（0或1值）
     */
    void setNavBits(const std::vector<uint8_t>& bits);

    /**
     * @brief 生成伪随机导航电文（用于测试）
     *
     * 生成指定帧数的伪随机导航比特数据
     *
     * @param num_frames 帧数
     */
    void generateRandomNavData(size_t num_frames = 1);

    /**
     * @brief 获取当前导航比特序列长度
     * @return size_t 比特数量
     */
    size_t getNavBitsLength() const { return m_navBits.size(); }

    /**
     * @brief 获取PRN号
     * @return uint8_t PRN号
     */
    uint8_t getPrn() const { return m_prn; }

    /**
     * @brief 获取电文类型
     * @return BdsMessageType 电文类型
     */
    BdsMessageType getMessageType() const { return m_messageType; }

    /**
     * @brief 获取每比特的PRN周期数
     * @return size_t PRN周期数
     */
    size_t getPrnPeriodsPerBit() const {
        return (m_messageType == BdsMessageType::D1) ? D1_PRN_PERIODS_PER_BIT : D2_PRN_PERIODS_PER_BIT;
    }

    /**
     * @brief 是否使用NH码
     * @return bool true表示使用NH码
     */
    bool usesNHCode() const {
        return m_messageType == BdsMessageType::D1;
    }

    /**
     * @brief 设置电文类型
     * @param message_type 新的电文类型
     * @param regenerate_random 是否重新生成随机比特（默认false，保留已有数据）
     */
    void setMessageType(BdsMessageType message_type, bool regenerate_random = false);

private:
    uint8_t m_prn;                       // 卫星PRN号
    BdsMessageType m_messageType;        // 电文类型
    std::vector<int8_t> m_navBits;       // 导航比特序列（+1/-1格式）
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_B1I_NAV_BIT_GENERATOR_H