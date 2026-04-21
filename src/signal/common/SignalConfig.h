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

namespace startrace {
namespace signal {

/**
 * @brief 信号输出模式
 */
enum class OutputMode {
    REAL_IF,         // 实数中频模式：输出单通道 float 数组 (S = A * cos(...))
    COMPLEX_BASEBAND // 复数基带模式：输出 I/Q 交织的 float 数组 (S = I + jQ)
};

/**
 * @brief 北斗电文类型
 */
enum class BdsMessageType {
    D1,  // MEO/IGSO卫星电文，速率 50 bps，含 NH 码
    D2   // GEO卫星电文，速率 500 bps，不含 NH 码
};

/**
 * @brief B1I 流式信号生成器配置参数
 * 采用 C with Class 风格，均为 Plain Old Data，方便在模块间传递。
 */
struct SignalConfigB1I {
    // ==================== B1I 物理层常数 (不可修改) ====================
    // B1I 载波标称中心频率 (Hz)
    static constexpr double CARRIER_FREQ_HZ = 1561.098e6;
    // B1I 测距码标称码速率 (Chips/sec)
    static constexpr double CODE_RATE_CPS   = 2.046e6;
    // B1I 每个 PRN 周期的码片长度 (Chips)
    static constexpr int    CODE_LENGTH     = 2046;
    // 合法的 PRN 编号范围
    static constexpr int    MIN_PRN         = 1;
    static constexpr int    MAX_PRN         = 63;

    // ==================== 流式调度参数 ====================
    // 每一块(Chunk)生成的采样点数量。
    // 建议设为 1024, 2048 或 4096，以适配 CPU 缓存和系统管道(Pipe)缓冲区。
    int chunk_size = 4096;

    // ==================== 卫星与采样基础配置 ====================
    // 卫星公用 PRN 编号 (1 - 63)
    int    prn_number               = 1;
    // 接收机采样频率 (Hz)，例如 16.368 MHz
    double sample_rate_hz           = 16.368e6;
    // 数字中频 (Hz)，仅在 OutputMode::REAL_IF 模式下生效
    double intermediate_freq_hz     = 4.092e6;

    // ==================== 信号质量与噪声 ====================
    // 载噪比 C/N0 (dB-Hz)，决定了信号中添加噪声的强度
    double cn0_dbhz                 = 45.0;
    // 纯净信号的幅度 A。
    // 在无噪情况下，实中频信号峰值为 A，复基带信号功率为 A^2
    double signal_amplitude         = 1.0;
    // 是否启用 AWGN (高斯白噪声) 叠加
    bool   enable_noise             = true;
    // 噪声生成器的随机种子。
    // 固定此值可以确保在多次运行中生成的噪声序列完全一致，便于调试算法。
    uint32_t noise_seed             = 42;

    // ==================== 初始相位与动态特性 ====================
    // t=0 时刻的初始载波相位 (弧度, 0 ~ 2π)
    double initial_carrier_phase_rad = 0.0;
    // t=0 时刻的初始码相位 (Chips, 0 ~ 2046)
    double initial_code_phase_chips  = 0.0;
    // 载波多普勒频移 (Hz)
    double doppler_freq_hz           = 0.0;
    // 多普勒变化率 (Hz/s)，用于模拟卫星与接收机间的相对加速度
    double doppler_rate_hz_per_s     = 0.0;

    // ==================== 接收机前端滤波 ====================
    // 是否启用前端带宽限制滤波器 (IIR 差分方程模式)
    bool   enable_frontend_filter   = false;
    // 前端低通/带通滤波器的单边带宽 (Hz)。
    // 例如设为 2.046e6 表示模拟一个约 4MHz 射频带宽的前端。
    double frontend_bandwidth_hz    = 2.046e6;

    // ==================== 输出模式配置 ====================
    // 信号输出类型：实中频 或 复基带
    OutputMode     output_mode  = OutputMode::REAL_IF;
    // 电文协议类型：D1 或 D2
    BdsMessageType message_type = BdsMessageType::D1;
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_SIGNAL_CONFIG_H