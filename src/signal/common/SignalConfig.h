/*
******************************************************************************
* @file           : SignalConfig.h
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : B1I信号配置参数
* @date           : 2026/4/19
******************************************************************************
*/
#ifndef STARTRACE_SIGNAL_CONFIG_H
#define STARTRACE_SIGNAL_CONFIG_H

#include <cstdint>
#include <cstddef>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace startrace {
namespace signal {

/**
 * @brief 信号输出模式枚举
 */
enum class OutputMode {
    REAL_IF,         // 实数中频模式：s(t) = A * c(t) * d(t) * cos(2π(f_IF+f_d)t + φ)
    COMPLEX_BASEBAND // 复数基带模式：s(t) = A * c(t) * d(t) * exp(j(2π f_d t + φ))
};

/**
 * @brief BDS电文类型枚举
 */
enum class BdsMessageType {
    D1,  // MEO/IGSO卫星电文，使用NH码，符号率50sps
    D2   // GEO卫星电文，不使用NH码，符号率500sps
};

/**
 * @brief 输出数据类型（量化位数）
 */
enum class SampleFormat {
    FLOAT32,  // 浮点（默认）
    INT16,    // 16-bit 量化
    INT8      // 8-bit 量化（贴近真实ADC）
};

/**
 * @brief B1I信号生成器配置参数
 */
struct SignalConfigB1I {
    // ==================== B1I信号固有参数（不可修改）====================
    static constexpr double CARRIER_FREQ_HZ = 1561.098e6;    // B1I载波频率
    static constexpr double CODE_RATE_CPS   = 2.046e6;       // 码速率 (chips/s)
    static constexpr size_t CODE_LENGTH_CHIPS = 2046;        // PRN码长度
    static constexpr double CODE_PERIOD_S   = 1.0e-3;        // 码周期 = 1ms
    static constexpr double D1_SYMBOL_RATE_SPS = 50.0;
    static constexpr double D2_SYMBOL_RATE_SPS = 500.0;
    static constexpr size_t NH_CODE_LENGTH  = 20;

    // ==================== 用户可配置参数 ====================
    int    prn_number               = 1;              // 卫星PRN编号 (1-63)
    double sample_rate_hz           = 16.368e6;       // 采样率 (Hz)
    double intermediate_freq_hz     = 4.092e6;        // 中频 (Hz)，仅实中频模式使用
    double signal_duration_s        = 1.0;            // 信号时长 (秒)

    double cn0_dbhz                 = 45.0;           // 载噪比 (dB-Hz)
    double signal_amplitude         = 1.0;            // 信号幅度 A (归一化)
    bool   enable_noise             = true;           // 是否添加AWGN噪声
    uint32_t noise_seed             = 42;             // 噪声随机种子（可复现）

    double initial_carrier_phase_rad = 0.0;           // 初始载波相位 (弧度)
    double initial_code_phase_chips  = 0.0;           // 初始码相位 (chips)
    double doppler_freq_hz          = 0.0;            // 多普勒频移 (Hz)
    double doppler_rate_hz_per_s    = 0.0;            // 多普勒变化率 (Hz/s)

    // 前端带宽滤波
    bool   enable_frontend_filter   = false;          // 是否启用前端带宽滤波
    double frontend_bandwidth_hz    = 4.092e6;        // 接收机前端带宽 (Hz)

    // 输出量化
    SampleFormat sample_format      = SampleFormat::FLOAT32;
    double quantize_full_scale      = 3.0;            // 量化时的满量程（单位：标准差的倍数）

    OutputMode     output_mode   = OutputMode::REAL_IF;
    BdsMessageType message_type  = BdsMessageType::D1;

    // ==================== 辅助计算方法 ====================
    size_t getTotalSamples() const {
        return static_cast<size_t>(sample_rate_hz * signal_duration_s + 0.5);
    }

    double getCodePhaseStep() const {
        return CODE_RATE_CPS / sample_rate_hz;
    }

    double getCarrierPhaseStep() const {
        double total_freq = (output_mode == OutputMode::REAL_IF)
                           ? (intermediate_freq_hz + doppler_freq_hz)
                           : doppler_freq_hz;
        return 2.0 * M_PI * total_freq / sample_rate_hz;
    }

    double getSymbolRate() const {
        return (message_type == BdsMessageType::D1)
               ? D1_SYMBOL_RATE_SPS : D2_SYMBOL_RATE_SPS;
    }

    size_t getCodePeriodsPerBit() const {
        return (message_type == BdsMessageType::D1) ? 20 : 2;
    }

    /**
     * @brief 信号功率（用于CN0噪声计算）
     * 复基带 BPSK: C = A^2
     * 实中频 BPSK: C = A^2 / 2（余弦的时间平均）
     */
    double getSignalPower() const {
        double A = signal_amplitude;
        return (output_mode == OutputMode::COMPLEX_BASEBAND) ? (A * A) : (A * A / 2.0);
    }
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_SIGNAL_CONFIG_H