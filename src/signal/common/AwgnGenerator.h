/*
******************************************************************************
* @file           : AwgnGenerator.h
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : 接收机前端带宽滤波器（理想砖墙FFT滤波，便于仿真）
* @attention      : None
* @date           : 2026/4/19
******************************************************************************
*/
#ifndef STARTRACE_FRONTEND_FILTER_H
#define STARTRACE_FRONTEND_FILTER_H

#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

namespace startrace {
namespace signal {

/**
 * @brief 前端带宽滤波器（基于FFT的理想砖墙滤波）
 *
 * 注意：本实现为便于仿真使用的朴素DFT版本，长序列建议接入FFTW/KissFFT。
 * 对长度 N 的复序列，复杂度 O(N^2)。
 *
 * 若 N 较大（>65536），请务必替换为FFT实现。
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
        std::vector<std::complex<double>> X(N), x(N);
        for (size_t i = 0; i < N; ++i)
            x[i] = std::complex<double>(samples[i].real(), samples[i].imag());

        dft(x, X, false);

        // 频率 bin: k -> f_k = k*fs/N (k<N/2), (k-N)*fs/N (k>=N/2)
        const double half_bw = bandwidth_hz / 2.0;
        for (size_t k = 0; k < N; ++k) {
            double f = (k < N / 2) ? (double)k * fs / N
                                    : ((double)k - (double)N) * fs / N;
            if (std::abs(f) > half_bw) X[k] = 0.0;
        }

        dft(X, x, true);
        for (size_t i = 0; i < N; ++i)
            samples[i] = std::complex<float>((float)x[i].real(), (float)x[i].imag());
    }

    /**
     * @brief 实中频带通滤波（保留 |f - f_if| <= bw/2 和其镜像）
     */
    static void applyBandpassReal(std::vector<float>& samples,
                                  double fs, double center_freq, double bandwidth_hz) {
        const size_t N = samples.size();
        if (N == 0) return;
        std::vector<std::complex<double>> X(N), x(N);
        for (size_t i = 0; i < N; ++i) x[i] = std::complex<double>(samples[i], 0.0);

        dft(x, X, false);

        const double half_bw = bandwidth_hz / 2.0;
        for (size_t k = 0; k < N; ++k) {
            double f = (k < N / 2) ? (double)k * fs / N
                                    : ((double)k - (double)N) * fs / N;
            bool keep = (std::abs(std::abs(f) - center_freq) <= half_bw);
            if (!keep) X[k] = 0.0;
        }

        dft(X, x, true);
        for (size_t i = 0; i < N; ++i) samples[i] = (float)x[i].real();
    }

private:
    // 朴素DFT（仅用于小规模仿真；生产请用FFT库）
    static void dft(const std::vector<std::complex<double>>& in,
                    std::vector<std::complex<double>>& out, bool inverse) {
        const size_t N = in.size();
        const double sign = inverse ? 2.0 * M_PI : -2.0 * M_PI;
        for (size_t k = 0; k < N; ++k) {
            std::complex<double> s(0.0, 0.0);
            for (size_t n = 0; n < N; ++n) {
                double angle = sign * (double)k * (double)n / (double)N;
                s += in[n] * std::complex<double>(std::cos(angle), std::sin(angle));
            }
            out[k] = inverse ? s / (double)N : s;
        }
    }
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_FRONTEND_FILTER_H
