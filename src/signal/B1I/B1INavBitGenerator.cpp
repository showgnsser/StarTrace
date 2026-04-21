/*
******************************************************************************
* @file           : B1INavBitGenerator.cpp
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : B1I导航电文比特生成器实现（支持D1/D2电文）
* @attention      : None
* @date           : 2026/4/19
******************************************************************************
*/
#include "B1INavBitGenerator.h"
#include <random>

namespace startrace {
namespace signal {

B1INavBitGenerator::B1INavBitGenerator(int prn, BdsMessageType message_type)
    : m_prn(prn)
    , m_messageType(message_type)
{
    if (m_prn < 1 || m_prn > 63) {
        m_prn = 1; // 越界保护，防御性降级
    }
    // 默认生成1帧伪随机数据作为初始缓冲
    generateRandomNavData(1);
}

int8_t B1INavBitGenerator::getModulationValue(int prn_period_index) const {
    int8_t nav_bit = getNavBit(prn_period_index);

    if (m_messageType == BdsMessageType::D1) {
        // D1电文：内联查表获取NH码，直接相乘
        return NHCode::getChipForPrnPeriod(prn_period_index) * nav_bit;
    } else {
        // D2电文：无NH码调制
        return nav_bit;
    }
}

int8_t B1INavBitGenerator::getNavBit(int prn_period_index) const {
    if (m_navBits.empty()) {
        return 1; // 默认输出持续的载波波峰
    }

    int prn_periods_per_bit = (m_messageType == BdsMessageType::D1)
                              ? D1_PRN_PERIODS_PER_BIT
                              : D2_PRN_PERIODS_PER_BIT;

    // 计算当前处于哪个比特索引，并进行循环取模
    int bit_index = (prn_period_index / prn_periods_per_bit) % m_navBits.size();
    return m_navBits[bit_index];
}

void B1INavBitGenerator::setNavBits(const std::vector<uint8_t>& bits) {
    m_navBits.clear();
    // 预分配内存，防止大电文文件载入时发生多次堆拷贝
    m_navBits.reserve(bits.size());

    // 一次性映射为 +1 / -1
    for (size_t i = 0; i < bits.size(); ++i) {
        m_navBits.push_back((bits[i] == 0) ? 1 : -1);
    }
}

void B1INavBitGenerator::generateRandomNavData(int num_frames) {
    int bits_per_frame = (m_messageType == BdsMessageType::D1)
                         ? D1_BITS_PER_FRAME
                         : D2_BITS_PER_FRAME;

    int total_bits = num_frames * bits_per_frame;

    // 使用 C++11 随机数发生器，仅在初始化时调用，不影响流式性能
    uint32_t seed = m_prn * 12345 + static_cast<uint32_t>(m_messageType) * 67890;
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, 1);

    m_navBits.clear();
    m_navBits.reserve(total_bits);

    for (int i = 0; i < total_bits; ++i) {
        m_navBits.push_back((dist(gen) == 0) ? 1 : -1);
    }
}

void B1INavBitGenerator::setMessageType(BdsMessageType message_type, bool regenerate_random) {
    if (m_messageType != message_type) {
        m_messageType = message_type;
        if (regenerate_random) {
            generateRandomNavData(1);
        }
    }
}

} // namespace signal
} // namespace startrace