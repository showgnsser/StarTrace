/*
******************************************************************************
* @file           : IIRFilter.cpp
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : None
* @attention      : None
* @date           : 2026/4/21
******************************************************************************
*/

#include "IIRFilter.h"

namespace startrace {
namespace signal {

IIRFilter::IIRFilter(double b0, double b1, double b2, double a1, double a2)
    : m_b0(b0), m_b1(b1), m_b2(b2), m_a1(a1), m_a2(a2)
{
    reset();
}

void IIRFilter::reset() {
    m_x1 = 0.0;
    m_x2 = 0.0;
    m_y1 = 0.0;
    m_y2 = 0.0;
}

void IIRFilter::processChunk(float* buffer, int length, int stride, int offset) {
    if (length <= 0 || buffer == nullptr) return;

    double x1 = m_x1, x2 = m_x2, y1 = m_y1, y2 = m_y2;
    double b0 = m_b0, b1 = m_b1, b2 = m_b2, a1 = m_a1, a2 = m_a2;

    for (int i = 0; i < length; ++i) {
        // 利用步长和偏移，直接在交织数组中提取/写入数据
        int idx = offset + i * stride;
        double x0 = static_cast<double>(buffer[idx]);
        double y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        x2 = x1; x1 = x0; y2 = y1; y1 = y0;
        buffer[idx] = static_cast<float>(y0);
    }

    m_x1 = x1; m_x2 = x2; m_y1 = y1; m_y2 = y2;
}

} // namespace signal
} // namespace startrace
