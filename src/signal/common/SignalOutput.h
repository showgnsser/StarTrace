/*
******************************************************************************
* @file           : SignalOutput.h
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : None
* @attention      : None
* @date           : 2026/4/19
******************************************************************************
*/
#ifndef STARTRACE_SIGNAL_OUTPUT_H
#define STARTRACE_SIGNAL_OUTPUT_H

#include "SignalConfig.h"
#include <complex>
#include <cstdint>
#include <vector>

namespace startrace {
namespace signal {

/**
 * @brief 信号输出容器
 *
 * 根据 OutputMode:
 *   - COMPLEX_BASEBAND -> 使用 complex_samples (I+jQ)
 *   - REAL_IF          -> 使用 real_samples
 *
 * 根据 SampleFormat 不同，对应的 _i8 / _i16 字段会被填充。
 */
struct SignalOutput {
    OutputMode   mode            = OutputMode::COMPLEX_BASEBAND;
    SampleFormat format          = SampleFormat::FLOAT32;
    double       sample_rate_hz  = 0.0;
    double       cn0_dbhz        = 0.0;
    double       noise_sigma     = 0.0;   // 每实数分量的噪声标准差（调试用）
    double       signal_power    = 0.0;   // 信号功率（调试用）
    size_t       num_samples     = 0;

    // FLOAT32
    std::vector<std::complex<float>> complex_samples; // 复基带
    std::vector<float>               real_samples;    // 实中频

    // INT16
    std::vector<int16_t> complex_samples_i16;         // 交织 [I0,Q0,I1,Q1,...]
    std::vector<int16_t> real_samples_i16;

    // INT8
    std::vector<int8_t>  complex_samples_i8;          // 交织
    std::vector<int8_t>  real_samples_i8;
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_SIGNAL_OUTPUT_H
