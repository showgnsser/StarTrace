/*
******************************************************************************
* @file           : FrontendFilter.h
* @author         : showskills
* @brief          : 接收机前端带宽滤波器（FFT频域砖墙滤波，基于PocketFFT）
* @date           : 2026/4/19
*
* 内部实现从朴素 O(N^2) DFT 升级为 PocketFFT 的 O(N log N) FFT。
* 公共接口保持不变，调用方无需修改任何代码。
******************************************************************************
*/
#ifndef STARTRACE_FRONTEND_FILTER_H
#define STARTRACE_FRONTEND_FILTER_H

#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

// PocketFFT: header-only, BSD-licensed
// 仓库: https://gitlab.mpcdf.mpg.de/mtr/pocketfft (cpp 分支)
#include "pocketfft_hdronly.h"

namespace startrace {
namespace signal {

/**
 * @brief 前端带宽滤波器（基于FFT的理想砖墙滤波）
 *
 * 内部使用 PocketFFT，复杂度 O(N log N)。对任意长度 N 都能高效处理，
 * 不要求 N 是 2 的幂。建议大数据量（> 1M 样本）可再配合流式/分块处理。
 */
class FrontendFilter {
public:
    /**
     * @brief 复基带低通滤波（保留 |f| <= bandwidth/2）
     */
    static void applyLowpassComplex(std::vector<std::complex<float>>& samples,
                                    double fs, double bandwidth_hz) {
        const size_t N = samples.size();
        if (N == 0) return;

        // 复制到双精度缓冲区（PocketFFT 支持 float，但 GNSS 仿真推荐 double）
        std::vector<std::complex<double>> buf(N);
        for (size_t i = 0; i < N; ++i) {
            buf[i] = std::complex<double>(samples[i].real(), samples[i].imag());
        }

        // 前向 FFT（原地）
        fftInplace(buf, /*inverse=*/false);

        // 频域砖墙 mask：k -> f_k = k*fs/N (k<N/2), (k-N)*fs/N (k>=N/2)
        const double half_bw = bandwidth_hz / 2.0;
        const double df = fs / static_cast<double>(N);
        for (size_t k = 0; k < N; ++k) {
            double f = (k < N / 2) ? static_cast<double>(k) * df
                                    : (static_cast<double>(k) - static_cast<double>(N)) * df;
            if (std::abs(f) > half_bw) buf[k] = std::complex<double>(0.0, 0.0);
        }

        // 反向 FFT（原地，已包含 1/N 归一化）
        fftInplace(buf, /*inverse=*/true);

        // 写回 float
        for (size_t i = 0; i < N; ++i) {
            samples[i] = std::complex<float>(static_cast<float>(buf[i].real()),
                                             static_cast<float>(buf[i].imag()));
        }
    }

    /**
     * @brief 实中频带通滤波（保留 |f - f_if| <= bw/2 和其镜像）
     */
    static void applyBandpassReal(std::vector<float>& samples,
                                  double fs, double center_freq, double bandwidth_hz) {
        const size_t N = samples.size();
        if (N == 0) return;

        // 实信号打包为复数，直接做 c2c（实现简单、与原逻辑一致）
        // 优化版可用 r2c/c2r，内存和速度减半；此处保持与原接口等价。
        std::vector<std::complex<double>> buf(N);
        for (size_t i = 0; i < N; ++i) {
            buf[i] = std::complex<double>(static_cast<double>(samples[i]), 0.0);
        }

        fftInplace(buf, /*inverse=*/false);

        const double half_bw = bandwidth_hz / 2.0;
        const double df = fs / static_cast<double>(N);
        for (size_t k = 0; k < N; ++k) {
            double f = (k < N / 2) ? static_cast<double>(k) * df
                                    : (static_cast<double>(k) - static_cast<double>(N)) * df;
            bool keep = (std::abs(std::abs(f) - center_freq) <= half_bw);
            if (!keep) buf[k] = std::complex<double>(0.0, 0.0);
        }

        fftInplace(buf, /*inverse=*/true);

        // 实信号的带通结果仍为实数，虚部应为数值误差（~1e-15）
        for (size_t i = 0; i < N; ++i) {
            samples[i] = static_cast<float>(buf[i].real());
        }
    }

private:
    /**
     * @brief PocketFFT 原地 c2c FFT 封装
     * @param data    输入/输出缓冲区（原地变换）
     * @param inverse false=前向变换, true=反向变换（带 1/N 归一化）
     */
    static void fftInplace(std::vector<std::complex<double>>& data, bool inverse) {
        const size_t N = data.size();
        if (N <= 1) return;

        const pocketfft::shape_t  shape  = { N };
        const pocketfft::stride_t stride = { static_cast<std::ptrdiff_t>(sizeof(std::complex<double>)) };
        const pocketfft::shape_t  axes   = { 0 };

        // PocketFFT 前向/反向不自带归一化，反向时手动乘 1/N
        const double scale = inverse ? (1.0 / static_cast<double>(N)) : 1.0;

        pocketfft::c2c(
            shape, stride, stride, axes,
            inverse ? pocketfft::BACKWARD : pocketfft::FORWARD,
            data.data(), data.data(),
            scale
        );
    }
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_FRONTEND_FILTER_H