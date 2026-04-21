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

#include "SignalConfig.h"
#include "SignalOutput.h"
#include "PrnCodeB1I.h"
#include "B1INavBitGenerator.h"
#include "IIRFilter.h"
#include "AwgnGenerator.h"
#include <vector>

namespace startrace {
namespace signal {

class B1ISignalModulator {
public:
    /**
     * @brief 构造函数，传入配置并初始化所有内部状态和子模块
     */
    explicit B1ISignalModulator(const SignalConfigB1I& config);

    /**
     * @brief 核心方法：流式生成下一块信号数据
     * @param chunk 外部传入的缓冲区视图，要求 chunk.data 已经分配好足够的内存
     */
    void generateNextChunk(ChunkBuffer& chunk);

    /**
     * @brief 复位所有相位和时间状态，回到 t=0 的时刻
     */
    void reset();

private:
    // 内部常量
    static constexpr double TWO_PI = 6.28318530717958647692;

    // ==================== 模块与配置 ====================
    SignalConfigB1I m_config;
    std::vector<int8_t> m_prnCode;           // 测距码序列缓存
    B1INavBitGenerator m_navBitGen;          // 电文生成器
    IIRFilter m_filter_I;                    // 用于 实中频 或 复基带的 I 路
    IIRFilter m_filter_Q;                    // 用于 复基带的 Q 路
    AwgnGenerator m_noiseGen;

    // ==================== 流式状态记忆 ====================
    double m_codePhase;                      // 码相位 (chips)
    double m_carrierPhase;                   // 载波相位 (rad)
    int    m_prnPeriodCount;                 // 已经历的完整PRN周期数
    long long m_totalSamplesGenerated;       // 全局采样点计数器 (用于计算多普勒变化率 t)

    // ==================== 辅助预计算变量 ====================
    double m_baseCodeStep;                   // 基础码相位步进
    double m_dt;                             // 采样间隔

    // 内部辅助函数
    void initFilter();
    inline void wrapPhase(double& phase);
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_B1I_SIGNAL_MODULATOR_H
