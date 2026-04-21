/**
 * @file    test_signal_modulator.cpp
 * @brief   B1ISignalModulator 单元测试（彻底适配零拷贝、流式 Chunk 架构）
 */
#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "signal/B1I/B1ISignalModulator.h"

using namespace startrace::signal;

class SignalModulatorTest : public ::testing::Test {
protected:
    static double calculatePowerReal(const float* buffer, int length) {
        if (length <= 0) return 0.0;
        double sum = 0.0;
        for (int i = 0; i < length; ++i) {
            sum += static_cast<double>(buffer[i]) * buffer[i];
        }
        return sum / length;
    }

    static SignalConfigB1I baseConfig() {
        SignalConfigB1I cfg;
        cfg.prn_number = 6;
        cfg.chunk_size = 1024;  // 每次测试生成 1024 点
        cfg.sample_rate_hz = 16.368e6;
        cfg.intermediate_freq_hz = 4.092e6;
        cfg.cn0_dbhz = 45.0;
        cfg.signal_amplitude = 1.0;
        cfg.enable_noise = false; // 默认无噪
        cfg.enable_frontend_filter = false;
        cfg.output_mode = OutputMode::REAL_IF;
        return cfg;
    }
};

TEST_F(SignalModulatorTest, GenerateChunk_BasicExecution) {
    SignalConfigB1I cfg = baseConfig();
    B1ISignalModulator mod(cfg);

    float buffer[1024] = {0};
    ChunkBuffer chunk;
    chunk.data = buffer;

    // 执行第一次流式切片
    mod.generateNextChunk(chunk);

    EXPECT_EQ(chunk.num_samples, 1024);
    // 检查相位被正确记录到了元数据中
    EXPECT_DOUBLE_EQ(chunk.current_code_phase, 0.0);
    EXPECT_DOUBLE_EQ(chunk.current_carrier_phase, 0.0);

    // 功率验证：实中频 BPSK 功率近似为 A^2 / 2 = 0.5
    double p = calculatePowerReal(chunk.data, chunk.num_samples);
    EXPECT_NEAR(p, 0.5, 0.05);
}

TEST_F(SignalModulatorTest, GenerateChunk_PhaseContinuity) {
    SignalConfigB1I cfg = baseConfig();
    B1ISignalModulator mod(cfg);

    float buffer[1024];
    ChunkBuffer chunk;
    chunk.data = buffer;

    mod.generateNextChunk(chunk);
    double endPhase1 = chunk.current_carrier_phase; // 此块的起始相位

    mod.generateNextChunk(chunk);
    double endPhase2 = chunk.current_carrier_phase; // 下一块的起始相位

    // 两块的起始相位必然不同（状态发生了延续）
    EXPECT_NE(endPhase1, endPhase2);
}

TEST_F(SignalModulatorTest, GenerateChunk_NoiseAdded) {
    SignalConfigB1I cfg = baseConfig();
    cfg.enable_noise = true;
    cfg.cn0_dbhz = 35.0; // 强噪声

    B1ISignalModulator mod(cfg);
    float buffer[1024];
    ChunkBuffer chunk;
    chunk.data = buffer;

    mod.generateNextChunk(chunk);

    // 总功率应显著大于纯净信号功率 (0.5)
    double p = calculatePowerReal(chunk.data, chunk.num_samples);
    EXPECT_GT(p, 0.6);
}

TEST_F(SignalModulatorTest, GenerateChunk_NoiseReproducibility) {
    SignalConfigB1I cfg = baseConfig();
    cfg.enable_noise = true;
    cfg.noise_seed = 12345;

    // Modulator 1
    B1ISignalModulator mod1(cfg);
    float buf1[1024];
    ChunkBuffer chunk1; chunk1.data = buf1;
    mod1.generateNextChunk(chunk1);

    // Modulator 2 (相同种子)
    B1ISignalModulator mod2(cfg);
    float buf2[1024];
    ChunkBuffer chunk2; chunk2.data = buf2;
    mod2.generateNextChunk(chunk2);

    // 验证噪声的一致性
    for (int i = 0; i < 1024; ++i) {
        EXPECT_FLOAT_EQ(buf1[i], buf2[i]);
    }
}

TEST_F(SignalModulatorTest, GenerateChunk_FrontendFilterActive) {
    SignalConfigB1I cfg = baseConfig();
    cfg.enable_frontend_filter = true;
    cfg.frontend_bandwidth_hz = 1.0e6; // 窄带滤波

    B1ISignalModulator mod_filtered(cfg);
    cfg.enable_frontend_filter = false;
    B1ISignalModulator mod_clean(cfg);

    float buf_filtered[1024]; ChunkBuffer chunkF; chunkF.data = buf_filtered;
    float buf_clean[1024];    ChunkBuffer chunkC; chunkC.data = buf_clean;

    mod_filtered.generateNextChunk(chunkF);
    mod_clean.generateNextChunk(chunkC);

    // 滤波后，由于高频被削弱，总功率通常会低于未滤波的信号
    double p_filtered = calculatePowerReal(chunkF.data, chunkF.num_samples);
    double p_clean = calculatePowerReal(chunkC.data, chunkC.num_samples);

    EXPECT_LT(p_filtered, p_clean);
}

TEST_F(SignalModulatorTest, ResetRestoresState) {
    SignalConfigB1I cfg = baseConfig();
    B1ISignalModulator mod(cfg);

    float buffer[1024];
    ChunkBuffer chunk;
    chunk.data = buffer;

    // 1. 第一次调用：记录的是第 1 块开始时的相位（此时必定为 0）
    mod.generateNextChunk(chunk);
    EXPECT_DOUBLE_EQ(chunk.current_code_phase, 0.0);
    EXPECT_DOUBLE_EQ(chunk.current_carrier_phase, 0.0);

    // 2. 第二次调用：记录的是第 2 块开始时的相位（经过上一次生成，现在绝对不再是 0）
    mod.generateNextChunk(chunk);
    EXPECT_NE(chunk.current_code_phase, 0.0);
    EXPECT_NE(chunk.current_carrier_phase, 0.0);

    // 3. 触发复位！
    mod.reset();

    // 4. 复位后第一次调用：起始状态应该完美恢复为 0
    mod.generateNextChunk(chunk);
    EXPECT_DOUBLE_EQ(chunk.current_code_phase, 0.0);
    EXPECT_DOUBLE_EQ(chunk.current_carrier_phase, 0.0);
}