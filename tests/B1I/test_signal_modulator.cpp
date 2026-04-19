/**
 * @file    test_signal_modulator.cpp
 * @brief   B1ISignalModulator 单元测试（适配重构后代码）
 */
#include <gtest/gtest.h>
#include <cmath>
#include "signal/B1I/B1ISignalModulator.h"

namespace startrace {
namespace signal {
namespace test {

class SignalModulatorTest : public ::testing::Test {
protected:
    static constexpr double SAMPLE_RATE = 16.368e6;
    static constexpr double IF_FREQ     = 4.092e6;
    static constexpr double PI          = 3.14159265358979323846;

    static double calculatePowerComplex(const std::vector<std::complex<float>>& signal) {
        if (signal.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : signal) sum += std::norm(s);
        return sum / signal.size();
    }

    static double calculatePowerReal(const std::vector<float>& signal) {
        if (signal.empty()) return 0.0;
        double sum = 0.0;
        for (float s : signal) sum += static_cast<double>(s) * s;
        return sum / signal.size();
    }

    // 基础 cfg：复基带、无噪声、短时长，便于快速测试
    static SignalConfigB1I baseConfig(double duration = 0.001) {
        SignalConfigB1I cfg;
        cfg.prn_number          = 6;
        cfg.sample_rate_hz      = SAMPLE_RATE;
        cfg.intermediate_freq_hz= IF_FREQ;
        cfg.signal_duration_s   = duration;
        cfg.cn0_dbhz            = 45.0;
        cfg.signal_amplitude    = 1.0;
        cfg.enable_noise        = false;
        cfg.doppler_freq_hz     = 0.0;
        cfg.doppler_rate_hz_per_s = 0.0;
        cfg.enable_frontend_filter = false;
        cfg.output_mode         = OutputMode::COMPLEX_BASEBAND;
        cfg.message_type        = BdsMessageType::D1;
        cfg.sample_format       = SampleFormat::FLOAT32;
        return cfg;
    }
};

// ==================== 构造函数测试 ====================

TEST_F(SignalModulatorTest, ConstructorD1) {
    B1ISignalModulator mod(6, SAMPLE_RATE, BdsMessageType::D1);

    EXPECT_EQ(mod.getPrn(), 6);
    EXPECT_DOUBLE_EQ(mod.getSampleRate(), SAMPLE_RATE);
    EXPECT_EQ(mod.getMessageType(), BdsMessageType::D1);
    EXPECT_FALSE(mod.isGeoSatellite());
}

TEST_F(SignalModulatorTest, ConstructorD2) {
    B1ISignalModulator mod(1, SAMPLE_RATE, BdsMessageType::D2);

    EXPECT_EQ(mod.getMessageType(), BdsMessageType::D2);
    EXPECT_TRUE(mod.isGeoSatellite());
}

TEST_F(SignalModulatorTest, ConstructorDefaultMessageType) {
    // 默认电文类型为 D1
    B1ISignalModulator mod(10, SAMPLE_RATE);
    EXPECT_EQ(mod.getMessageType(), BdsMessageType::D1);
}

TEST_F(SignalModulatorTest, InvalidParameters) {
    EXPECT_THROW(B1ISignalModulator mod(0, SAMPLE_RATE), std::invalid_argument);
    EXPECT_THROW(B1ISignalModulator mod(6, 0.0),         std::invalid_argument);
    EXPECT_THROW(B1ISignalModulator mod(6, -1.0),        std::invalid_argument);
}

// ==================== 旧接口：信号生成测试（向后兼容） ====================

TEST_F(SignalModulatorTest, Legacy_GenerateByDuration) {
    B1ISignalModulator mod(6, SAMPLE_RATE);

    auto signal = mod.generate(0.001);
    size_t expected = static_cast<size_t>(SAMPLE_RATE * 0.001 + 0.5);

    EXPECT_EQ(signal.size(), expected);
}

TEST_F(SignalModulatorTest, Legacy_GenerateBySamples) {
    B1ISignalModulator mod(6, SAMPLE_RATE);
    auto signal = mod.generateSamples(1000);
    EXPECT_EQ(signal.size(), 1000u);
}

TEST_F(SignalModulatorTest, Legacy_SignalPower) {
    B1ISignalModulator mod(6, SAMPLE_RATE);

    auto signal = mod.generate(0.01);
    double power = calculatePowerComplex(signal);

    // 旧接口在 generateContinuous 中 A=1：|I|^2+|Q|^2 = cos^2+sin^2 = 1
    EXPECT_NEAR(power, 1.0, 0.01);
}

TEST_F(SignalModulatorTest, Legacy_ZeroDopplerRealAxis) {
    B1ISignalModulator mod(6, SAMPLE_RATE);

    auto signal = mod.generate(0.001, 0.0, 0.0, 0.0);

    double sumI = 0.0, sumQ = 0.0;
    for (const auto& s : signal) {
        sumI += std::abs(s.real());
        sumQ += std::abs(s.imag());
    }
    EXPECT_LT(sumQ / sumI, 0.1);
}

TEST_F(SignalModulatorTest, Legacy_DopplerShiftEffect) {
    B1ISignalModulator mod(6, SAMPLE_RATE);

    auto signal = mod.generate(0.01, 1000.0);

    double sumQ = 0.0;
    for (const auto& s : signal) sumQ += std::abs(s.imag());
    EXPECT_GT(sumQ, 0.0);
}

TEST_F(SignalModulatorTest, Legacy_ContinuousGeneration) {
    B1ISignalModulator mod(6, SAMPLE_RATE);

    mod.generateContinuous(1000, 0.0);
    size_t count1 = mod.getPrnPeriodCount();

    mod.generateContinuous(1000, 0.0);
    size_t count2 = mod.getPrnPeriodCount();

    EXPECT_GE(count2, count1);
}

TEST_F(SignalModulatorTest, Legacy_Reset) {
    B1ISignalModulator mod(6, SAMPLE_RATE);

    mod.generateContinuous(10000, 1000.0);
    mod.reset();

    EXPECT_DOUBLE_EQ(mod.getCodePhase(),    0.0);
    EXPECT_DOUBLE_EQ(mod.getCarrierPhase(), 0.0);
    EXPECT_EQ(mod.getPrnPeriodCount(), 0u);
}

TEST_F(SignalModulatorTest, Legacy_DifferentPrnsDifferentSignals) {
    B1ISignalModulator mod1(6, SAMPLE_RATE);
    B1ISignalModulator mod2(7, SAMPLE_RATE);

    auto s1 = mod1.generateSamples(1000, 0.0, 0.0, 0.0);
    auto s2 = mod2.generateSamples(1000, 0.0, 0.0, 0.0);

    int diffCount = 0;
    for (size_t i = 0; i < s1.size(); ++i) {
        if (std::abs(s1[i].real() - s2[i].real()) > 0.1) ++diffCount;
    }
    EXPECT_GT(diffCount, 100);
}

TEST_F(SignalModulatorTest, Legacy_SamePrnSameSignal) {
    B1ISignalModulator mod1(6, SAMPLE_RATE);
    B1ISignalModulator mod2(6, SAMPLE_RATE);

    auto s1 = mod1.generateSamples(100, 0.0, 0.0, 0.0);
    auto s2 = mod2.generateSamples(100, 0.0, 0.0, 0.0);

    for (size_t i = 0; i < s1.size(); ++i) {
        EXPECT_FLOAT_EQ(s1[i].real(), s2[i].real());
    }
}

// ==================== 新接口：generateSignal(cfg) 基本测试 ====================

TEST_F(SignalModulatorTest, GenerateSignal_ComplexBasebandBasic) {
    SignalConfigB1I cfg = baseConfig(0.001);

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    EXPECT_EQ(out.mode,           OutputMode::COMPLEX_BASEBAND);
    EXPECT_EQ(out.format,         SampleFormat::FLOAT32);
    EXPECT_EQ(out.num_samples,    cfg.getTotalSamples());
    EXPECT_DOUBLE_EQ(out.sample_rate_hz, SAMPLE_RATE);

    // 复基带：应填充 complex_samples，不填 real_samples
    EXPECT_EQ(out.complex_samples.size(), cfg.getTotalSamples());
    EXPECT_TRUE(out.real_samples.empty());
}

TEST_F(SignalModulatorTest, GenerateSignal_RealIfBasic) {
    SignalConfigB1I cfg = baseConfig(0.001);
    cfg.output_mode = OutputMode::REAL_IF;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    EXPECT_EQ(out.mode, OutputMode::REAL_IF);
    EXPECT_EQ(out.real_samples.size(), cfg.getTotalSamples());
    EXPECT_TRUE(out.complex_samples.empty());
}

// ==================== 信号功率测试 ====================

TEST_F(SignalModulatorTest, GenerateSignal_ComplexPower_NoNoise) {
    SignalConfigB1I cfg = baseConfig(0.005);
    cfg.signal_amplitude = 1.0;
    cfg.enable_noise     = false;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    // 复基带 BPSK: P = A^2 = 1
    double p = calculatePowerComplex(out.complex_samples);
    EXPECT_NEAR(p, 1.0, 0.02);
    EXPECT_NEAR(out.signal_power, 1.0, 1e-6);
}

TEST_F(SignalModulatorTest, GenerateSignal_RealPower_NoNoise) {
    SignalConfigB1I cfg = baseConfig(0.005);
    cfg.output_mode      = OutputMode::REAL_IF;
    cfg.signal_amplitude = 1.0;
    cfg.enable_noise     = false;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    // 实中频 BPSK: P = A^2 / 2 = 0.5（余弦时间平均）
    double p = calculatePowerReal(out.real_samples);
    EXPECT_NEAR(p, 0.5, 0.02);
    EXPECT_NEAR(out.signal_power, 0.5, 1e-6);
}

TEST_F(SignalModulatorTest, GenerateSignal_AmplitudeScales) {
    SignalConfigB1I cfg = baseConfig(0.002);
    cfg.signal_amplitude = 2.0;
    cfg.enable_noise     = false;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    // 复基带：P = A^2 = 4
    double p = calculatePowerComplex(out.complex_samples);
    EXPECT_NEAR(p, 4.0, 0.1);
}

// ==================== 多普勒测试 ====================

TEST_F(SignalModulatorTest, GenerateSignal_ZeroDopplerComplexOnRealAxis) {
    SignalConfigB1I cfg = baseConfig(0.001);
    cfg.doppler_freq_hz = 0.0;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    double sumI = 0.0, sumQ = 0.0;
    for (const auto& s : out.complex_samples) {
        sumI += std::abs(s.real());
        sumQ += std::abs(s.imag());
    }
    EXPECT_LT(sumQ / sumI, 0.01);   // 初始相位为 0 时 Q 应几乎为 0
}

TEST_F(SignalModulatorTest, GenerateSignal_NonZeroDopplerHasQ) {
    SignalConfigB1I cfg = baseConfig(0.01);
    cfg.doppler_freq_hz = 1500.0;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    double sumQ = 0.0;
    for (const auto& s : out.complex_samples) sumQ += std::abs(s.imag());
    EXPECT_GT(sumQ, 0.0);
}

TEST_F(SignalModulatorTest, GenerateSignal_DopplerRateEffective) {
    // 有多普勒变化率时，最终码相位应比常值多普勒时略有差异
    SignalConfigB1I cfg1 = baseConfig(0.02);
    cfg1.doppler_freq_hz = 0.0;
    cfg1.doppler_rate_hz_per_s = 0.0;

    SignalConfigB1I cfg2 = cfg1;
    cfg2.doppler_rate_hz_per_s = 1000.0;   // 较大变化率，便于检测

    B1ISignalModulator mod1(cfg1.prn_number, cfg1.sample_rate_hz, cfg1.message_type);
    B1ISignalModulator mod2(cfg2.prn_number, cfg2.sample_rate_hz, cfg2.message_type);

    auto out1 = mod1.generateSignal(cfg1);
    auto out2 = mod2.generateSignal(cfg2);

    // 两者应不相同
    bool anyDiff = false;
    for (size_t i = 0; i < out1.complex_samples.size(); ++i) {
        if (std::abs(out1.complex_samples[i].imag()
                   - out2.complex_samples[i].imag()) > 0.01) {
            anyDiff = true;
            break;
        }
    }
    EXPECT_TRUE(anyDiff);
}

// ==================== CN0 噪声测试 ====================

TEST_F(SignalModulatorTest, GenerateSignal_NoiseAddedComplex) {
    SignalConfigB1I cfg = baseConfig(0.01);
    cfg.enable_noise = true;
    cfg.cn0_dbhz     = 35.0;   // 较低 CN0，噪声明显

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    // noise_sigma 应为正值
    EXPECT_GT(out.noise_sigma, 0.0);

    // 总功率 = C + N0*fs ≈ A^2 + 2*sigma^2
    double total_power  = calculatePowerComplex(out.complex_samples);
    double expected_pow = cfg.getSignalPower() + 2.0 * out.noise_sigma * out.noise_sigma;
    // 容差宽松，统计波动
    EXPECT_NEAR(total_power, expected_pow, 0.1 * expected_pow);
}

TEST_F(SignalModulatorTest, GenerateSignal_NoiseAddedReal) {
    SignalConfigB1I cfg = baseConfig(0.01);
    cfg.output_mode = OutputMode::REAL_IF;
    cfg.enable_noise = true;
    cfg.cn0_dbhz     = 40.0;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    EXPECT_GT(out.noise_sigma, 0.0);

    // 实中频总功率 ≈ A^2/2 + sigma^2
    double total_power  = calculatePowerReal(out.real_samples);
    double expected_pow = cfg.getSignalPower() + out.noise_sigma * out.noise_sigma;
    EXPECT_NEAR(total_power, expected_pow, 0.1 * expected_pow);
}

TEST_F(SignalModulatorTest, GenerateSignal_HigherCn0LowerSigma) {
    SignalConfigB1I cfg_low  = baseConfig(0.001);
    cfg_low.enable_noise = true;
    cfg_low.cn0_dbhz     = 30.0;

    SignalConfigB1I cfg_high = cfg_low;
    cfg_high.cn0_dbhz    = 50.0;

    B1ISignalModulator mod1(cfg_low.prn_number,  cfg_low.sample_rate_hz,  cfg_low.message_type);
    B1ISignalModulator mod2(cfg_high.prn_number, cfg_high.sample_rate_hz, cfg_high.message_type);

    auto o1 = mod1.generateSignal(cfg_low);
    auto o2 = mod2.generateSignal(cfg_high);

    // CN0 越高 sigma 越小
    EXPECT_GT(o1.noise_sigma, o2.noise_sigma);
}

TEST_F(SignalModulatorTest, GenerateSignal_NoiseDisabled) {
    SignalConfigB1I cfg = baseConfig(0.001);
    cfg.enable_noise = false;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    EXPECT_DOUBLE_EQ(out.noise_sigma, 0.0);
}

TEST_F(SignalModulatorTest, GenerateSignal_ReproducibleWithSameSeed) {
    SignalConfigB1I cfg = baseConfig(0.002);
    cfg.enable_noise = true;
    cfg.noise_seed   = 12345;

    B1ISignalModulator m1(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    B1ISignalModulator m2(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);

    auto o1 = m1.generateSignal(cfg);
    auto o2 = m2.generateSignal(cfg);

    ASSERT_EQ(o1.complex_samples.size(), o2.complex_samples.size());
    for (size_t i = 0; i < o1.complex_samples.size(); ++i) {
        EXPECT_FLOAT_EQ(o1.complex_samples[i].real(), o2.complex_samples[i].real());
        EXPECT_FLOAT_EQ(o1.complex_samples[i].imag(), o2.complex_samples[i].imag());
    }
}

TEST_F(SignalModulatorTest, GenerateSignal_DifferentSeedDifferentNoise) {
    SignalConfigB1I cfg = baseConfig(0.002);
    cfg.enable_noise = true;

    cfg.noise_seed = 111;
    B1ISignalModulator m1(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    auto o1 = m1.generateSignal(cfg);

    cfg.noise_seed = 999;
    B1ISignalModulator m2(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    auto o2 = m2.generateSignal(cfg);

    // 至少应该有不同样本
    bool anyDiff = false;
    for (size_t i = 0; i < o1.complex_samples.size(); ++i) {
        if (o1.complex_samples[i] != o2.complex_samples[i]) { anyDiff = true; break; }
    }
    EXPECT_TRUE(anyDiff);
}

// ==================== 量化输出测试 ====================

TEST_F(SignalModulatorTest, GenerateSignal_Int16ComplexFormat) {
    SignalConfigB1I cfg = baseConfig(0.001);
    cfg.sample_format = SampleFormat::INT16;
    cfg.enable_noise  = true;
    cfg.cn0_dbhz      = 45.0;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    EXPECT_EQ(out.format, SampleFormat::INT16);
    // 复基带 INT16 交织 I0 Q0 I1 Q1 ...
    EXPECT_EQ(out.complex_samples_i16.size(), 2 * cfg.getTotalSamples());
    EXPECT_TRUE(out.complex_samples.empty());

    // 值在 int16 范围内
    for (int16_t v : out.complex_samples_i16) {
        EXPECT_GE(v, -32768);
        EXPECT_LE(v,  32767);
    }
}

TEST_F(SignalModulatorTest, GenerateSignal_Int8RealFormat) {
    SignalConfigB1I cfg = baseConfig(0.001);
    cfg.output_mode   = OutputMode::REAL_IF;
    cfg.sample_format = SampleFormat::INT8;
    cfg.enable_noise  = true;
    cfg.cn0_dbhz      = 40.0;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    EXPECT_EQ(out.format, SampleFormat::INT8);
    EXPECT_EQ(out.real_samples_i8.size(), cfg.getTotalSamples());
    EXPECT_TRUE(out.real_samples.empty());

    for (int8_t v : out.real_samples_i8) {
        EXPECT_GE(v, -128);
        EXPECT_LE(v,  127);
    }
}

TEST_F(SignalModulatorTest, GenerateSignal_Float32DoesNotQuantize) {
    SignalConfigB1I cfg = baseConfig(0.001);
    cfg.sample_format = SampleFormat::FLOAT32;

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    EXPECT_FALSE(out.complex_samples.empty());
    EXPECT_TRUE(out.complex_samples_i16.empty());
    EXPECT_TRUE(out.complex_samples_i8.empty());
}

// ==================== 电文类型与 D1/D2 测试 ====================

TEST_F(SignalModulatorTest, GenerateSignal_D1VsD2DifferentSignal) {
    // 相同 PRN，不同电文类型，信号应有差异（至少因调制值不同）
    SignalConfigB1I cfgD1 = baseConfig(0.02);
    cfgD1.prn_number  = 6;
    cfgD1.message_type = BdsMessageType::D1;

    SignalConfigB1I cfgD2 = cfgD1;
    cfgD2.message_type = BdsMessageType::D2;

    B1ISignalModulator m1(cfgD1.prn_number, cfgD1.sample_rate_hz, cfgD1.message_type);
    B1ISignalModulator m2(cfgD2.prn_number, cfgD2.sample_rate_hz, cfgD2.message_type);

    auto o1 = m1.generateSignal(cfgD1);
    auto o2 = m2.generateSignal(cfgD2);

    int diff = 0;
    for (size_t i = 0; i < o1.complex_samples.size(); ++i) {
        if (std::abs(o1.complex_samples[i].real()
                   - o2.complex_samples[i].real()) > 0.1) ++diff;
    }
    EXPECT_GT(diff, 100);
}

TEST_F(SignalModulatorTest, SetMessageType_UpdatesInternalState) {
    B1ISignalModulator mod(6, SAMPLE_RATE, BdsMessageType::D1);
    EXPECT_EQ(mod.getMessageType(), BdsMessageType::D1);

    mod.setMessageType(BdsMessageType::D2);
    EXPECT_EQ(mod.getMessageType(), BdsMessageType::D2);
    EXPECT_TRUE(mod.isGeoSatellite());
}

// ==================== SignalConfigB1I 辅助方法测试 ====================

TEST_F(SignalModulatorTest, Config_TotalSamples) {
    SignalConfigB1I cfg = baseConfig(0.010);  // 10 ms
    EXPECT_EQ(cfg.getTotalSamples(),
              static_cast<size_t>(SAMPLE_RATE * 0.010 + 0.5));
}

TEST_F(SignalModulatorTest, Config_CodePhaseStep) {
    SignalConfigB1I cfg = baseConfig();
    EXPECT_NEAR(cfg.getCodePhaseStep(),
                SignalConfigB1I::CODE_RATE_CPS / SAMPLE_RATE, 1e-12);
}

TEST_F(SignalModulatorTest, Config_SignalPowerModes) {
    SignalConfigB1I cfg = baseConfig();
    cfg.signal_amplitude = 2.0;

    cfg.output_mode = OutputMode::COMPLEX_BASEBAND;
    EXPECT_NEAR(cfg.getSignalPower(), 4.0, 1e-9);      // A^2

    cfg.output_mode = OutputMode::REAL_IF;
    EXPECT_NEAR(cfg.getSignalPower(), 2.0, 1e-9);      // A^2/2
}

TEST_F(SignalModulatorTest, Config_SymbolRateAndPeriodsPerBit) {
    SignalConfigB1I cfg;
    cfg.message_type = BdsMessageType::D1;
    EXPECT_DOUBLE_EQ(cfg.getSymbolRate(), 50.0);
    EXPECT_EQ(cfg.getCodePeriodsPerBit(), 20u);

    cfg.message_type = BdsMessageType::D2;
    EXPECT_DOUBLE_EQ(cfg.getSymbolRate(), 500.0);
    EXPECT_EQ(cfg.getCodePeriodsPerBit(), 2u);
}

// ==================== 前端带宽滤波测试（小规模 DFT） ====================

TEST_F(SignalModulatorTest, GenerateSignal_FrontendFilterReducesOutOfBand) {
    // 由于当前 FrontendFilter 是 O(N^2) 朴素 DFT，只用极短时长
    SignalConfigB1I cfg;
    cfg.prn_number             = 6;
    cfg.sample_rate_hz         = 2.046e6;   // 较低采样率，N 较小
    cfg.intermediate_freq_hz   = 0.0;
    cfg.signal_duration_s      = 0.0005;    // ~1023 点
    cfg.cn0_dbhz               = 45.0;
    cfg.signal_amplitude       = 1.0;
    cfg.enable_noise           = false;
    cfg.doppler_freq_hz        = 0.0;
    cfg.doppler_rate_hz_per_s  = 0.0;
    cfg.output_mode            = OutputMode::COMPLEX_BASEBAND;
    cfg.message_type           = BdsMessageType::D1;
    cfg.sample_format          = SampleFormat::FLOAT32;
    cfg.enable_frontend_filter = true;
    cfg.frontend_bandwidth_hz  = 1.0e6;     // 带宽小于信号主瓣

    B1ISignalModulator mod(cfg.prn_number, cfg.sample_rate_hz, cfg.message_type);
    SignalOutput out = mod.generateSignal(cfg);

    EXPECT_EQ(out.complex_samples.size(), cfg.getTotalSamples());

    // 滤波后功率应小于未滤波(=1.0)
    double p = calculatePowerComplex(out.complex_samples);
    EXPECT_GT(p, 0.0);
    EXPECT_LT(p, 1.0);
}

} // namespace test
} // namespace signal
} // namespace startrace