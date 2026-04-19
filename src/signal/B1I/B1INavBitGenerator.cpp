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
#include <stdexcept>

namespace startrace {
namespace signal {

B1INavBitGenerator::B1INavBitGenerator(uint8_t prn, BdsMessageType message_type)
    : m_prn(prn)
    , m_messageType(message_type)
{
    if (prn < 1 || prn > 63) {
        throw std::invalid_argument("PRN number must be between 1 and 63");
    }
    // 默认生成一帧的伪随机数据
    generateRandomNavData(1);
}

int8_t B1INavBitGenerator::getModulationValue(size_t prn_period_index) const {
    // 获取导航比特值
    int8_t nav_bit = getNavBit(prn_period_index);

    if (m_messageType == BdsMessageType::D1) {
        // D1电文：使用NH码调制
        int8_t nh_value = NHCode::getChipForPrnPeriod(prn_period_index);
        return static_cast<int8_t>(nh_value * nav_bit);
    } else {
        // D2电文：不使用NH码，直接返回导航比特
        return nav_bit;
    }
}

int8_t B1INavBitGenerator::getNavBit(size_t prn_period_index) const {
    if (m_navBits.empty()) {
        return 1; // 默认返回+1
    }

    // 根据电文类型计算每比特的PRN周期数
    size_t prn_periods_per_bit = getPrnPeriodsPerBit();

    // 计算对应的导航比特索引
    size_t bit_index = prn_period_index / prn_periods_per_bit;

    // 循环使用导航比特数据
    bit_index = bit_index % m_navBits.size();

    return m_navBits[bit_index];
}

void B1INavBitGenerator::setNavBits(const std::vector<uint8_t>& bits) {
    m_navBits.clear();
    m_navBits.reserve(bits.size());

    // 将0/1格式转换为+1/-1格式
    // 约定：0 -> +1, 1 -> -1（与BPSK调制一致）
    for (uint8_t bit : bits) {
        m_navBits.push_back(bit == 0 ? 1 : -1);
    }
}

void B1INavBitGenerator::generateRandomNavData(size_t num_frames) {
    size_t total_bits = num_frames * BITS_PER_FRAME;

    // 使用PRN号和电文类型作为随机种子的一部分，保证可重复性
    uint32_t seed = m_prn * 12345 + static_cast<uint32_t>(m_messageType) * 67890;
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dist(0, 1);

    m_navBits.clear();
    m_navBits.reserve(total_bits);

    for (size_t i = 0; i < total_bits; ++i) {
        // 随机生成0或1，然后转换为+1或-1
        m_navBits.push_back(dist(gen) == 0 ? 1 : -1);
    }
}

void B1INavBitGenerator::setMessageType(BdsMessageType message_type, bool regenerate_random) {
    if (m_messageType != message_type) {
        m_messageType = message_type;
        if (regenerate_random) {
            generateRandomNavData(1);
        }
        // 默认保留现有导航比特数据，避免用户已设置的数据被意外清空
    }
}

} // namespace signal
} // namespace startrace