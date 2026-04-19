/*
******************************************************************************
* @file           : B1ISignalModulator.cpp
* @author         : showskills
* @brief          : B1I信号调制器实现
* @date           : 2026/4/19
******************************************************************************
*/
#include "B1ISignalModulator.h"
#include "AwgnGenerator.h"
#include "FrontendFilter.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace startrace {
namespace signal {

namespace {
    constexpr double TWO_PI = 2.0 * M_PI;

    inline void wrapPhase(double& phase) {
        // 仅在幅度过大时归一化，减小 fmod 调用次数
        if (phase > 1e6 || phase < -1e6) {
            phase = std::fmod(phase, TWO_PI);
        }
    }
}

// ============================== 构造 ==============================
B1ISignalModulator::B1ISignalModulator(uint8_t prn, double sample_rate, BdsMessageType message_type)
    : m_prn(prn)
    , m_sampleRate(sample_rate)
    , m_codePhase(0.0)
    , m_carrierPhase(0.0)
    , m_prnPeriodCount(0)
    , m_sampleCount(0)
    , m_messageType(message_type)
    , m_navBitGen(std::make_unique<B1INavBitGenerator>(prn, message_type))
{
    if (prn < PrnCodeB1I::MIN_PRN || prn > PrnCodeB1I::MAX_PRN)
        throw std::invalid_argument("PRN number out of valid range");
    if (sample_rate <= 0)
        throw std::invalid_argument("Sample rate must be positive");

    m_prnCode = PrnCodeB1I::generate(prn);
}

// ============================== 一步式生成接口 ==============================
SignalOutput B1ISignalModulator::generateSignal(const SignalConfigB1I& cfg) {
    // 采样率一致性
    m_sampleRate = cfg.sample_rate_hz;

    // 电文类型同步
    if (cfg.message_type != m_messageType) {
        m_messageType = cfg.message_type;
        m_navBitGen->setMessageType(cfg.message_type, /*regenerate_random=*/false);
    }

    SignalOutput out;
    out.mode           = cfg.output_mode;
    out.format         = cfg.sample_format;
    out.sample_rate_hz = cfg.sample_rate_hz;
    out.cn0_dbhz       = cfg.cn0_dbhz;
    out.num_samples    = cfg.getTotalSamples();
    out.signal_power   = cfg.getSignalPower();

    // 1) 生成纯净信号
    std::vector<std::complex<float>> buf_c;
    std::vector<float>               buf_r;
    if (cfg.output_mode == OutputMode::COMPLEX_BASEBAND) {
        generateCleanComplex(cfg, buf_c);
    } else {
        generateCleanReal(cfg, buf_r);
    }

    // 2) 前端带宽滤波（在噪声之前：只滤信号；若希望滤"信号+噪声"把顺序调换）
    if (cfg.enable_frontend_filter) {
        if (cfg.output_mode == OutputMode::COMPLEX_BASEBAND) {
            FrontendFilter::applyLowpassComplex(buf_c, cfg.sample_rate_hz,
                                                cfg.frontend_bandwidth_hz);
        } else {
            FrontendFilter::applyBandpassReal(buf_r, cfg.sample_rate_hz,
                                              cfg.intermediate_freq_hz,
                                              cfg.frontend_bandwidth_hz);
        }
    }

    // 3) 添加AWGN噪声（基于CN0）
    double sigma = 0.0;
    if (cfg.enable_noise) {
        AwgnGenerator noise(cfg.noise_seed);
        if (cfg.output_mode == OutputMode::COMPLEX_BASEBAND) {
            sigma = noise.addNoiseComplex(buf_c, out.signal_power,
                                          cfg.cn0_dbhz, cfg.sample_rate_hz);
        } else {
            sigma = noise.addNoiseReal(buf_r, out.signal_power,
                                       cfg.cn0_dbhz, cfg.sample_rate_hz);
        }
    }
    out.noise_sigma = sigma;

    // 4) 量化/输出
    if (cfg.output_mode == OutputMode::COMPLEX_BASEBAND) {
        if (cfg.sample_format == SampleFormat::FLOAT32) {
            out.complex_samples = std::move(buf_c);
        } else {
            // 满量程：A + N*sigma（以标准差倍数估计）
            double full_scale = cfg.signal_amplitude
                              + cfg.quantize_full_scale * std::max(sigma, 1e-12);
            quantizeComplex(buf_c, out, full_scale);
        }
    } else {
        if (cfg.sample_format == SampleFormat::FLOAT32) {
            out.real_samples = std::move(buf_r);
        } else {
            double full_scale = cfg.signal_amplitude
                              + cfg.quantize_full_scale * std::max(sigma, 1e-12);
            quantizeReal(buf_r, out, full_scale);
        }
    }

    return out;
}

// ============================== 核心：纯净复基带 ==============================
void B1ISignalModulator::generateCleanComplex(const SignalConfigB1I& cfg,
                                              std::vector<std::complex<float>>& out)
{
    const size_t N  = cfg.getTotalSamples();
    const double fs = cfg.sample_rate_hz;
    const double dt = 1.0 / fs;
    const double A  = cfg.signal_amplitude;

    out.assign(N, std::complex<float>(0.0f, 0.0f));

    m_codePhase      = cfg.initial_code_phase_chips;
    m_carrierPhase   = cfg.initial_carrier_phase_rad;
    m_prnPeriodCount = 0;
    m_sampleCount    = 0;

    const double base_code_step = CODE_RATE_HZ / fs;

    for (size_t i = 0; i < N; ++i) {
        const double t = (double)i * dt;

        int8_t chip = getPrnChip(m_codePhase);
        int8_t mod  = m_navBitGen->getModulationValue(m_prnPeriodCount);
        double baseband = A * static_cast<double>(chip * mod);

        // 基带只含多普勒（瞬时）
        double fd = cfg.doppler_freq_hz + cfg.doppler_rate_hz_per_s * t;

        out[i] = std::complex<float>(
            static_cast<float>(baseband * std::cos(m_carrierPhase)),
            static_cast<float>(baseband * std::sin(m_carrierPhase))
        );

        // 相位积分
        m_carrierPhase += TWO_PI * fd * dt;
        wrapPhase(m_carrierPhase);

        // 码相位随多普勒按载波比例缩放
        double code_step = base_code_step * (1.0 + fd / CARRIER_FREQ_HZ);
        m_codePhase += code_step;
        while (m_codePhase >= (double)CODE_LENGTH) {
            m_codePhase -= (double)CODE_LENGTH;
            ++m_prnPeriodCount;
        }
        ++m_sampleCount;
    }
}

// ============================== 核心：纯净实中频 ==============================
void B1ISignalModulator::generateCleanReal(const SignalConfigB1I& cfg,
                                           std::vector<float>& out)
{
    const size_t N  = cfg.getTotalSamples();
    const double fs = cfg.sample_rate_hz;
    const double dt = 1.0 / fs;
    const double A  = cfg.signal_amplitude;
    const double fIF = cfg.intermediate_freq_hz;

    out.assign(N, 0.0f);

    m_codePhase      = cfg.initial_code_phase_chips;
    m_carrierPhase   = cfg.initial_carrier_phase_rad;
    m_prnPeriodCount = 0;
    m_sampleCount    = 0;

    const double base_code_step = CODE_RATE_HZ / fs;

    for (size_t i = 0; i < N; ++i) {
        const double t = (double)i * dt;

        int8_t chip = getPrnChip(m_codePhase);
        int8_t mod  = m_navBitGen->getModulationValue(m_prnPeriodCount);
        double baseband = A * static_cast<double>(chip * mod);

        // 实中频：f_total = f_IF + f_doppler(t)
        double fd = cfg.doppler_freq_hz + cfg.doppler_rate_hz_per_s * t;
        double f_total = fIF + fd;

        out[i] = static_cast<float>(baseband * std::cos(m_carrierPhase));

        m_carrierPhase += TWO_PI * f_total * dt;
        wrapPhase(m_carrierPhase);

        double code_step = base_code_step * (1.0 + fd / CARRIER_FREQ_HZ);
        m_codePhase += code_step;
        while (m_codePhase >= (double)CODE_LENGTH) {
            m_codePhase -= (double)CODE_LENGTH;
            ++m_prnPeriodCount;
        }
        ++m_sampleCount;
    }
}

// ============================== 量化 ==============================
void B1ISignalModulator::quantizeComplex(const std::vector<std::complex<float>>& in,
                                         SignalOutput& out, double full_scale)
{
    const size_t N = in.size();
    const double inv_fs = 1.0 / std::max(full_scale, 1e-12);

    if (out.format == SampleFormat::INT16) {
        out.complex_samples_i16.resize(2 * N);
        const double k = 32767.0 * inv_fs;
        for (size_t i = 0; i < N; ++i) {
            double qi = std::max(-32768.0, std::min(32767.0, in[i].real() * k));
            double qq = std::max(-32768.0, std::min(32767.0, in[i].imag() * k));
            out.complex_samples_i16[2 * i]     = static_cast<int16_t>(qi);
            out.complex_samples_i16[2 * i + 1] = static_cast<int16_t>(qq);
        }
    } else if (out.format == SampleFormat::INT8) {
        out.complex_samples_i8.resize(2 * N);
        const double k = 127.0 * inv_fs;
        for (size_t i = 0; i < N; ++i) {
            double qi = std::max(-128.0, std::min(127.0, in[i].real() * k));
            double qq = std::max(-128.0, std::min(127.0, in[i].imag() * k));
            out.complex_samples_i8[2 * i]     = static_cast<int8_t>(qi);
            out.complex_samples_i8[2 * i + 1] = static_cast<int8_t>(qq);
        }
    }
}

void B1ISignalModulator::quantizeReal(const std::vector<float>& in,
                                      SignalOutput& out, double full_scale)
{
    const size_t N = in.size();
    const double inv_fs = 1.0 / std::max(full_scale, 1e-12);

    if (out.format == SampleFormat::INT16) {
        out.real_samples_i16.resize(N);
        const double k = 32767.0 * inv_fs;
        for (size_t i = 0; i < N; ++i) {
            double q = std::max(-32768.0, std::min(32767.0, in[i] * k));
            out.real_samples_i16[i] = static_cast<int16_t>(q);
        }
    } else if (out.format == SampleFormat::INT8) {
        out.real_samples_i8.resize(N);
        const double k = 127.0 * inv_fs;
        for (size_t i = 0; i < N; ++i) {
            double q = std::max(-128.0, std::min(127.0, in[i] * k));
            out.real_samples_i8[i] = static_cast<int8_t>(q);
        }
    }
}

// ============================== 旧接口（兼容） ==============================
std::vector<std::complex<float>> B1ISignalModulator::generate(
    double duration_sec, double doppler_hz,
    double code_phase_chips, double carrier_phase_rad)
{
    size_t N = static_cast<size_t>(duration_sec * m_sampleRate + 0.5);
    return generateSamples(N, doppler_hz, code_phase_chips, carrier_phase_rad);
}

std::vector<std::complex<float>> B1ISignalModulator::generateSamples(
    size_t num_samples, double doppler_hz,
    double code_phase_chips, double carrier_phase_rad)
{
    m_codePhase      = code_phase_chips;
    m_carrierPhase   = carrier_phase_rad;
    m_prnPeriodCount = 0;
    m_sampleCount    = 0;
    return generateContinuous(num_samples, doppler_hz);
}

std::vector<std::complex<float>> B1ISignalModulator::generateContinuous(
    size_t num_samples, double doppler_hz)
{
    std::vector<std::complex<float>> signal(num_samples);

    double doppler_code_rate = CODE_RATE_HZ * (doppler_hz / CARRIER_FREQ_HZ);
    double code_phase_step   = (CODE_RATE_HZ + doppler_code_rate) / m_sampleRate;
    double carrier_phase_step = TWO_PI * doppler_hz / m_sampleRate;

    for (size_t i = 0; i < num_samples; ++i) {
        int8_t chip = getPrnChip(m_codePhase);
        int8_t mod  = m_navBitGen->getModulationValue(m_prnPeriodCount);
        double baseband = static_cast<double>(chip * mod);

        signal[i] = std::complex<float>(
            static_cast<float>(baseband * std::cos(m_carrierPhase)),
            static_cast<float>(baseband * std::sin(m_carrierPhase))
        );

        m_codePhase += code_phase_step;
        while (m_codePhase >= (double)CODE_LENGTH) {
            m_codePhase -= (double)CODE_LENGTH;
            ++m_prnPeriodCount;
        }

        m_carrierPhase += carrier_phase_step;
        wrapPhase(m_carrierPhase);
        ++m_sampleCount;
    }
    return signal;
}

// ============================== 其他 ==============================
void B1ISignalModulator::reset() {
    m_codePhase      = 0.0;
    m_carrierPhase   = 0.0;
    m_prnPeriodCount = 0;
    m_sampleCount    = 0;
}

void B1ISignalModulator::setNavBits(const std::vector<uint8_t>& bits) {
    m_navBitGen->setNavBits(bits);
}

void B1ISignalModulator::setMessageType(BdsMessageType message_type) {
    if (m_messageType != message_type) {
        m_messageType = message_type;
        m_navBitGen->setMessageType(message_type, /*regenerate_random=*/false);
    }
}

int8_t B1ISignalModulator::getPrnChip(double code_phase) const {
    double np = code_phase;
    if (np < 0.0 || np >= (double)CODE_LENGTH) {
        np = std::fmod(np, (double)CODE_LENGTH);
        if (np < 0.0) np += (double)CODE_LENGTH;
    }
    size_t idx = static_cast<size_t>(std::floor(np));
    if (idx >= CODE_LENGTH) idx = CODE_LENGTH - 1;
    return m_prnCode[idx];
}

} // namespace signal
} // namespace startrace