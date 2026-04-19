/*
******************************************************************************
* @file           : B1ISignalModulator.h
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : B1I信号调制器（支持D1/D2电文）
* @attention      : None
* @date           : 2026/4/19
******************************************************************************
*/
#ifndef STARTRACE_B1I_SIGNAL_MODULATOR_H
#define STARTRACE_B1I_SIGNAL_MODULATOR_H

#include "PrnCodeB1I.h"
#include "B1INavBitGenerator.h"
#include "SignalConfig.h"
#include "SignalOutput.h"
#include <cstdint>
#include <cstddef>
#include <complex>
#include <vector>
#include <memory>

namespace startrace {
namespace signal {

/**
 * @brief B1I信号调制器（支持D1和D2电文）
 *
 * 负责生成完整的B1I基带信号，包含：
 * - PRN码调制
 * - NH码调制（仅D1电文）
 * - 导航电文调制
 * - 载波多普勒频移
 * - 码相位控制
 *
 * B1I信号参数：
 * - 中心频率：1561.098 MHz
 * - 码速率：2.046 Mcps
 * - 码长度：2046 chips
 * - 码周期：1 ms
 * - 调制方式：BPSK(2)
 *
 * D1电文（MEO/IGSO）：50bps，使用NH码
 * D2电文（GEO）：500bps，不使用NH码
 *
 * 支持特性：
 *   - D1/D2 电文（D1 含 NH 码）
 *   - 复基带 / 实中频 输出
 *   - 多普勒频移 + 多普勒变化率
 *   - 基于 CN0 的 AWGN 噪声
 *   - 前端带宽滤波（可选）
 *   - 输出量化（FLOAT32 / INT16 / INT8）
 *
 * 使用推荐流程：用 SignalConfigB1I 构造并调用 generateSignal()，一步完成。
 */
class B1ISignalModulator {
public:
    // B1I信号常量
    static constexpr double CARRIER_FREQ_HZ = 1561.098e6;     // 载波频率
    static constexpr double CODE_RATE_HZ = 2.046e6;           // 码速率
    static constexpr size_t CODE_LENGTH = 2046;               // 码长度
    static constexpr double CODE_PERIOD_SEC = 0.001;          // 码周期 1ms
    static constexpr double CHIP_DURATION_SEC = 1.0 / CODE_RATE_HZ; // 码片持续时间

    /**
     * @brief 构造函数
     * @param prn 卫星PRN号（1-63）
     * @param sample_rate 采样率（Hz）
     * @param message_type 电文类型（D1或D2，默认D1）
     */
    B1ISignalModulator(uint8_t prn, double sample_rate,
                       BdsMessageType message_type = BdsMessageType::D1);

    /**
     * @brief 根据配置生成完整的B1I信号（包含噪声、滤波、量化）
     */
    SignalOutput generateSignal(const SignalConfigB1I& cfg);

    /**
     * @brief 生成指定时间长度的基带信号
     *
     * @param duration_sec 信号时长（秒）
     * @param doppler_hz 载波多普勒频移（Hz）
     * @param code_phase_chips 初始码相位（chips）
     * @param carrier_phase_rad 初始载波相位（弧度）
     * @return std::vector<std::complex<float>> 复基带信号（I+jQ）
     */
    std::vector<std::complex<float>> generate(
        double duration_sec,
        double doppler_hz = 0.0,
        double code_phase_chips = 0.0,
        double carrier_phase_rad = 0.0
    );

    /**
     * @brief 生成指定采样点数的基带信号
     *
     * @param num_samples 采样点数
     * @param doppler_hz 载波多普勒频移（Hz）
     * @param code_phase_chips 初始码相位（chips）
     * @param carrier_phase_rad 初始载波相位（弧度）
     * @return std::vector<std::complex<float>> 复基带信号（I+jQ）
     */
    std::vector<std::complex<float>> generateSamples(
        size_t num_samples,
        double doppler_hz = 0.0,
        double code_phase_chips = 0.0,
        double carrier_phase_rad = 0.0
    );

    /**
     * @brief 连续生成信号（保持内部状态）
     *
     * 用于流式处理，每次调用会从上次结束的状态继续
     *
     * @param num_samples 采样点数
     * @param doppler_hz 载波多普勒频移（Hz）
     * @return std::vector<std::complex<float>> 复基带信号
     */
    std::vector<std::complex<float>> generateContinuous(
        size_t num_samples,
        double doppler_hz = 0.0
    );

    /**
     * @brief 重置内部状态
     */
    void reset();

    /**
     * @brief 设置导航电文数据
     * @param bits 导航比特序列（0或1值）
     */
    void setNavBits(const std::vector<uint8_t>& bits);

    /**
     * @brief 设置电文类型
     * @param message_type 电文类型（D1或D2）
     */
    void setMessageType(BdsMessageType message_type);

    /**
     * @brief 获取PRN号
     */
    uint8_t getPrn() const { return m_prn; }

    /**
     * @brief 获取采样率
     */
    double getSampleRate() const { return m_sampleRate; }

    /**
     * @brief 获取当前码相位
     */
    double getCodePhase() const { return m_codePhase; }

    /**
     * @brief 获取当前载波相位
     */
    double getCarrierPhase() const { return m_carrierPhase; }

    /**
     * @brief 获取当前PRN周期计数
     */
    size_t getPrnPeriodCount() const { return m_prnPeriodCount; }

    /**
     * @brief 获取电文类型
     */
    BdsMessageType getMessageType() const { return m_messageType; }

    /**
     * @brief 判断是否为GEO卫星（使用D2电文）
     */
    bool isGeoSatellite() const { return m_messageType == BdsMessageType::D2; }

private:
    /**
     * @brief 获取指定码相位的PRN码值
     * @param code_phase 码相位（chips）
     * @return int8_t PRN码值（+1或-1）
     */
    int8_t getPrnChip(double code_phase) const;

    // 核心内部核函数：根据模式生成"纯净"信号（不加噪、不滤波、不量化）
    void generateCleanComplex(const SignalConfigB1I& cfg,
                              std::vector<std::complex<float>>& out);
    void generateCleanReal(const SignalConfigB1I& cfg,
                           std::vector<float>& out);

    // 量化辅助
    static void quantizeComplex(const std::vector<std::complex<float>>& in,
                                SignalOutput& out, double full_scale);
    static void quantizeReal(const std::vector<float>& in,
                             SignalOutput& out, double full_scale);

    uint8_t m_prn;                              // 卫星PRN号
    double m_sampleRate;                        // 采样率
    double m_codePhase;                         // 当前码相位（chips）
    double m_carrierPhase;                      // 当前载波相位（弧度）
    size_t m_prnPeriodCount;                    // PRN周期计数
    size_t m_sampleCount;                       // 总采样点计数
    BdsMessageType m_messageType;               // 电文类型

    std::vector<int8_t> m_prnCode;              // PRN码序列（缓存）
    std::unique_ptr<B1INavBitGenerator> m_navBitGen; // 导航电文生成器
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_B1I_SIGNAL_MODULATOR_H
