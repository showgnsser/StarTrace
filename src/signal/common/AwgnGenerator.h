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
#include <complex>
#include <cstdint>
#include <random>
#include <vector>

namespace startrace {
namespace signal {

/**
 * @brief 基于载噪比的AWGN噪声生成器
 *
 * CN0 定义：CN0(dB-Hz) = 10*log10(C / N0)
 * 给定信号功率 C、采样率 fs：
 *   每个实数分量的噪声方差 σ² = N0 * fs / 2
 */
class AwgnGenerator {
public:
    explicit AwgnGenerator(uint32_t seed = 42)
        : m_gen(seed), m_nd(0.0, 1.0) {}

    void reseed(uint32_t seed) { m_gen.seed(seed); }

    /**
     * @brief 计算每实数分量的噪声标准差
     * @param signal_power 信号功率 C（复基带用 A²，实中频用 A²/2）
     * @param cn0_dbhz     载噪比 dB-Hz
     * @param sample_rate  采样率 (Hz)
     */
    static double computeSigma(double signal_power, double cn0_dbhz,
                               double sample_rate) {
        double cn0_linear = std::pow(10.0, cn0_dbhz / 10.0);
        double n0 = signal_power / cn0_linear;
        return std::sqrt(n0 * sample_rate / 2.0);
    }

    /**
     * @brief 向复基带信号添加复高斯噪声 (I、Q 各 σ)
     */
    double addNoiseComplex(std::vector<std::complex<float>>& samples,
                           double signal_power, double cn0_dbhz, double fs) {
        double sigma = computeSigma(signal_power, cn0_dbhz, fs);
        for (auto& s : samples) {
            float ni = static_cast<float>(sigma * m_nd(m_gen));
            float nq = static_cast<float>(sigma * m_nd(m_gen));
            s.real(s.real() + ni);
            s.imag(s.imag() + nq);
        }
        return sigma;
    }

    /**
     * @brief 向实信号添加实高斯噪声
     */
    double addNoiseReal(std::vector<float>& samples,
                        double signal_power, double cn0_dbhz, double fs) {
        double sigma = computeSigma(signal_power, cn0_dbhz, fs);
        for (auto& s : samples) {
            s += static_cast<float>(sigma * m_nd(m_gen));
        }
        return sigma;
    }

private:
    std::mt19937 m_gen;
    std::normal_distribution<double> m_nd;
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_AWGN_GENERATOR_H