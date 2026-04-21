/*
******************************************************************************
* @file           : IIRFilter.h
* @author         : showskills
* @email          : gnsslh@whu.edu.cn
* @brief          : None
* @attention      : None
* @date           : 2026/4/21
******************************************************************************
*/
#ifndef STARTRACE_IIR_FILTER_H
#define STARTRACE_IIR_FILTER_H

namespace startrace {
namespace signal {

/**
 * @brief 状态记忆型二阶 IIR 差分方程滤波器
 * * 用于流式基带信号的实时滤波。
 * 差分方程：y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
 * （注：这里假设 a0 = 1.0）
 */
class IIRFilter {
public:
    /**
     * @brief 构造函数：初始化滤波器系数并清零历史状态
     * @param b0, b1, b2 分子系数 (前馈)
     * @param a1, a2 分母系数 (反馈，注意不包含 a0，且通常需要移项变号)
     */
    IIRFilter(double b0, double b1, double b2, double a1, double a2);

    /**
     * @brief 对连续的数据块进行就地滤波 (In-place filtering)
     * @param buffer 指向浮点数据块的指针 (输入/输出复用)
     * @param length 数据块长度
     */
    void processChunk(float* buffer, int length, int stride = 1, int offset = 0);

    /**
     * @brief 复位滤波器历史状态（用于切换通道或重新开始仿真）
     */
    void reset();

private:
    // 滤波器系数
    double m_b0, m_b1, m_b2;
    double m_a1, m_a2;

    // 历史状态记忆 (State Memory)
    double m_x1, m_x2; // x[n-1], x[n-2]
    double m_y1, m_y2; // y[n-1], y[n-2]
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_IIR_FILTER_H