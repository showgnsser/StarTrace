/*
******************************************************************************
* @file           : B1ISignalModulator.cpp
* @author         : showskills
* @brief          : B1I信号调制器实现
* @date           : 2026/4/19
******************************************************************************
*/
/*
******************************************************************************
* @file           : B1ISignalModulator.cpp
******************************************************************************
*/
#include "B1ISignalModulator.h"
#include <cmath>

namespace startrace {
namespace signal {

B1ISignalModulator::B1ISignalModulator(const SignalConfigB1I& config)
    : m_config(config)
    , m_navBitGen(config.prn_number, config.message_type)
    , m_filter_I(1.0, 0.0, 0.0, 0.0, 0.0) // 初始化 I 路滤波器
    , m_filter_Q(1.0, 0.0, 0.0, 0.0, 0.0) // 初始化 Q 路滤波器
    , m_noiseGen(config.noise_seed)
{
    m_prnCode = PrnCodeB1I::generate(m_config.prn_number);
    m_dt = 1.0 / m_config.sample_rate_hz;
    m_baseCodeStep = SignalConfigB1I::CODE_RATE_CPS * m_dt;

    initFilter();
    reset();
}

void B1ISignalModulator::reset() {
    m_codePhase = m_config.initial_code_phase_chips;
    m_carrierPhase = m_config.initial_carrier_phase_rad;
    m_prnPeriodCount = 0;
    m_totalSamplesGenerated = 0;
    m_filter_I.reset();
    m_filter_Q.reset();
}

inline void B1ISignalModulator::wrapPhase(double& phase) {
    while (phase >= TWO_PI) phase -= TWO_PI;
    while (phase < 0.0)     phase += TWO_PI;
}

void B1ISignalModulator::initFilter() {
    if (!m_config.enable_frontend_filter) return;

    double fs = m_config.sample_rate_hz;
    double fc = m_config.frontend_bandwidth_hz;
    double wc = std::tan(M_PI * fc / fs);
    double k  = wc;
    double q  = 0.7071067811865475; // 1/sqrt(2)
    double norm = 1.0 / (1.0 + k / q + k * k);

    double b0 = k * k * norm;
    double b1 = 2.0 * b0;
    double b2 = b0;
    double a1 = 2.0 * (k * k - 1.0) * norm;
    double a2 = (1.0 - k / q + k * k) * norm;

    // 两路滤波器使用相同的巴特沃斯系数
    m_filter_I = IIRFilter(b0, b1, b2, a1, a2);
    m_filter_Q = IIRFilter(b0, b1, b2, a1, a2);
}

void B1ISignalModulator::generateNextChunk(ChunkBuffer& chunk) {
    if (chunk.data == nullptr || m_config.chunk_size <= 0) return;

    chunk.current_code_phase = m_codePhase;
    chunk.current_carrier_phase = m_carrierPhase;
    chunk.num_samples = m_config.chunk_size;

    const double A = m_config.signal_amplitude;
    const double fIF = m_config.intermediate_freq_hz;
    const double fc = SignalConfigB1I::CARRIER_FREQ_HZ;
    const bool isComplex = (m_config.output_mode == OutputMode::COMPLEX_BASEBAND);

    double codePhase = m_codePhase;
    double carrierPhase = m_carrierPhase;
    int prnPeriodCount = m_prnPeriodCount;
    long long t_idx = m_totalSamplesGenerated;

    // =========================================================
    // 核心分离逻辑：避免在内层循环做条件判断，极大提升运行速度
    // =========================================================
    if (isComplex) {
        // 【复数基带模式】：生成 I/Q 交织序列 [I0, Q0, I1, Q1 ...]
        for (int i = 0; i < m_config.chunk_size; ++i) {
            double t = static_cast<double>(t_idx) * m_dt;
            // 复基带信号不包含中频 fIF，载波频率仅等于多普勒频移
            double fd = m_config.doppler_freq_hz + m_config.doppler_rate_hz_per_s * t;

            int chip_idx = static_cast<int>(codePhase);
            if (chip_idx >= SignalConfigB1I::CODE_LENGTH) chip_idx = SignalConfigB1I::CODE_LENGTH - 1;

            int8_t chip = m_prnCode[chip_idx];
            int8_t nav_mod = m_navBitGen.getModulationValue(prnPeriodCount);
            double baseband = A * static_cast<double>(chip * nav_mod);

            // S(t) = baseband * exp(j * phi) = I + jQ
            chunk.data[2 * i]     = static_cast<float>(baseband * std::cos(carrierPhase)); // I 路
            chunk.data[2 * i + 1] = static_cast<float>(baseband * std::sin(carrierPhase)); // Q 路

            carrierPhase += TWO_PI * fd * m_dt;
            wrapPhase(carrierPhase);

            double code_step = m_baseCodeStep * (1.0 + fd / fc);
            codePhase += code_step;
            while (codePhase >= SignalConfigB1I::CODE_LENGTH) {
                codePhase -= SignalConfigB1I::CODE_LENGTH;
                prnPeriodCount++;
            }
            t_idx++;
        }
    } else {
        // 【实数中频模式】：单通道序列
        for (int i = 0; i < m_config.chunk_size; ++i) {
            double t = static_cast<double>(t_idx) * m_dt;
            double fd = m_config.doppler_freq_hz + m_config.doppler_rate_hz_per_s * t;

            int chip_idx = static_cast<int>(codePhase);
            if (chip_idx >= SignalConfigB1I::CODE_LENGTH) chip_idx = SignalConfigB1I::CODE_LENGTH - 1;

            int8_t chip = m_prnCode[chip_idx];
            int8_t nav_mod = m_navBitGen.getModulationValue(prnPeriodCount);
            double baseband = A * static_cast<double>(chip * nav_mod);

            // S(t) = baseband * cos(2 * pi * (fIF + fd) * t)
            double f_total = fIF + fd;
            chunk.data[i] = static_cast<float>(baseband * std::cos(carrierPhase));

            carrierPhase += TWO_PI * f_total * m_dt;
            wrapPhase(carrierPhase);

            double code_step = m_baseCodeStep * (1.0 + fd / fc);
            codePhase += code_step;
            while (codePhase >= SignalConfigB1I::CODE_LENGTH) {
                codePhase -= SignalConfigB1I::CODE_LENGTH;
                prnPeriodCount++;
            }
            t_idx++;
        }
    }

    // =========================================================
    // 5. 应用状态记忆的 IIR 前端滤波器
    // =========================================================
    if (m_config.enable_frontend_filter) {
        if (isComplex) {
            // 复基带：步长为2，利用偏移分别滤 I 路(offset=0) 和 Q 路(offset=1)
            m_filter_I.processChunk(chunk.data, m_config.chunk_size, 2, 0);
            m_filter_Q.processChunk(chunk.data, m_config.chunk_size, 2, 1);
        } else {
            // 实中频：普通的连续数组，步长为1
            m_filter_I.processChunk(chunk.data, m_config.chunk_size, 1, 0);
        }
    }

    // =========================================================
    // 6. 叠加 AWGN 噪声
    // =========================================================
    if (m_config.enable_noise) {
        if (isComplex) {
            // 复基带总信号功率 C = A^2。
            // 巧合且优雅的数学性质：复基带中 I 和 Q 是两路独立的随机变量。
            // 我们直接将其视为一个长度为 2*chunk_size 的实数组加入噪声，
            // AWGN生成器根据 C=A^2 算出的每路方差恰好满足 N0 * fs / 2，逻辑完美自洽！
            double sig_power = A * A;
            m_noiseGen.addNoiseRealInPlace(
                chunk.data,
                m_config.chunk_size * 2, // 长度翻倍
                sig_power,
                m_config.cn0_dbhz,
                m_config.sample_rate_hz
            );
        } else {
            // 实中频 BPSK 的信号功率 C = A^2 / 2
            double sig_power = (A * A) / 2.0;
            m_noiseGen.addNoiseRealInPlace(
                chunk.data,
                m_config.chunk_size,
                sig_power,
                m_config.cn0_dbhz,
                m_config.sample_rate_hz
            );
        }
    }

    // 7. 将局部寄存器写回成员变量挂起，等待下一次切片
    m_codePhase = codePhase;
    m_carrierPhase = carrierPhase;
    m_prnPeriodCount = prnPeriodCount;
    m_totalSamplesGenerated = t_idx;
}

} // namespace signal
} // namespace startrace