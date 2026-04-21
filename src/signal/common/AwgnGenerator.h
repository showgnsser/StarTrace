/*
******************************************************************************
* @file           : AwgnGenerator.h
* @author         : showskills
* @brief          : 基于载噪比(CN0)的AWGN噪声生成器
* @date           : 2026/4/19
******************************************************************************
*/
#ifndef STARTRACE_AWGN_GENERATOR_H
#define STARTRACE_AWGN_GENERATOR_H

#include <cmath>
#include <random>

namespace startrace {
namespace signal {

/**
 * @brief 基于载噪比(CN0)的流式 AWGN 噪声生成器
 * 核心改造：提供就地缓冲区块处理能力，维持 PRNG 状态以保证流式噪声的白噪声特性。
 */
class AwgnGenerator {
public:
    explicit AwgnGenerator(uint32_t seed = 42)
        : m_gen(seed), m_nd(0.0, 1.0) {}

    void reseed(uint32_t seed) {
        m_gen.seed(seed);
        m_nd.reset(); // 清除正态分布的内部缓存
    }

    /**
     * @brief 为单通道实数信号就地添加高斯白噪声
     * @param buffer 浮点数据块指针
     * @param length 数据块长度
     * @param signal_power 信号功率 C（实中频用 A^2 / 2）
     * @param cn0_dbhz 载噪比 (dB-Hz)
     * @param sample_rate 采样率 (Hz)
     */
    void addNoiseRealInPlace(float* buffer, int length,
                             double signal_power, double cn0_dbhz, double sample_rate)
    {
        if (buffer == nullptr || length <= 0) return;

        double cn0_linear = std::pow(10.0, cn0_dbhz / 10.0);
        double n0 = signal_power / cn0_linear;
        // 计算标准差 sigma
        double sigma = std::sqrt(n0 * sample_rate / 2.0);

        for (int i = 0; i < length; ++i) {
            buffer[i] += static_cast<float>(sigma * m_nd(m_gen));
        }
    }

private:
    // 随机数引擎作为成员变量，保证在无限切片生成中，噪声序列永远不中断、不重复
    std::mt19937 m_gen;
    std::normal_distribution<double> m_nd;
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_AWGN_GENERATOR_H