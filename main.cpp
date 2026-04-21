#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "signal/B1I/B1ISignalModulator.h"
#include "SignalConfig.h"
#include "SignalOutput.h"

using namespace startrace::signal;

int main(int argc, char* argv[]) {
    // 1. 设置信号配置
    SignalConfigB1I config;
    config.prn_number = 6;
    config.sample_rate_hz = 16.368e6;
    config.intermediate_freq_hz = 4.092e6;
    config.doppler_freq_hz = 5000;
    config.cn0_dbhz = 35.0;
    config.chunk_size = 4096;
    config.enable_noise = true;
    config.noise_seed = 12345;
    config.enable_frontend_filter = true;
    config.frontend_bandwidth_hz = 4.092e6;

    // 【关键显式配置】：在这里切换你的输出模式
    // config.output_mode = OutputMode::REAL_IF;
    config.output_mode = OutputMode::COMPLEX_BASEBAND;

    double total_duration_s = 1.0;
    long long total_samples_needed = static_cast<long long>(config.sample_rate_hz * total_duration_s);

    B1ISignalModulator modulator(config);

    // 自动判断文件名后缀
    std::string filename = (config.output_mode == OutputMode::COMPLEX_BASEBAND)
                           ? "b1i_prn6_16M_complex.iq"
                           : "b1i_prn6_16M_real_if.bin";

    std::ofstream outFile(filename, std::ios::binary | std::ios::out);
    if (!outFile) {
        std::cerr << "无法创建输出文件: " << filename << std::endl;
        return -1;
    }

    // 2. 准备流式缓冲区 (栈上分配)
    // 【核心修复 1】：缓冲区必须按复基带的最大需求（2倍）开辟，防止栈溢出
    float buffer[4096 * 2] = {0};
    ChunkBuffer chunk;
    chunk.data = buffer;

    // 【核心修复 2】：判断每个物理样本占用几个 float
    int floats_per_sample = (config.output_mode == OutputMode::COMPLEX_BASEBAND) ? 2 : 1;

    std::cout << "开始生成 B1I 信号..." << std::endl;
    std::cout << "模式: " << (floats_per_sample == 2 ? "复数基带 (I/Q)" : "实数中频") << std::endl;

    long long samples_generated = 0;
    while (samples_generated < total_samples_needed) {

        modulator.generateNextChunk(chunk);

        // 【核心修复 3】：写入时，字节数必须乘以 floats_per_sample
        outFile.write(reinterpret_cast<const char*>(chunk.data),
                      chunk.num_samples * floats_per_sample * sizeof(float));

        samples_generated += chunk.num_samples;

        if ((samples_generated / chunk.num_samples) % 500 == 0) {
            double progress = (double)samples_generated / total_samples_needed * 100.0;
            std::cout << "\r进度: " << std::fixed << std::setprecision(1) << progress << "% " << std::flush;
        }
    }

    std::cout << "\r进度: 100.0%   完成！" << std::endl;
    std::cout << "信号已保存至: " << filename << std::endl;

    outFile.close();
    return 0;
}