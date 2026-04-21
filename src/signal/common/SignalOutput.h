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

namespace startrace {
namespace signal {

/**
 * @brief 流式数据块缓冲区描述符 (C-style Buffer Descriptor)
 * * 这是一个极简的“视图”结构体，它本身不管理内存，
 * 仅用于在生成器和调用者之间传递内存地址和状态信息。
 */
struct ChunkBuffer {
    // 指向外部预先分配好的浮点数组（避免内部 malloc/new）
    // - 如果是 REAL_IF 模式，长度应为 chunk_size
    // - 如果是 COMPLEX_BASEBAND 模式，长度应为 chunk_size * 2 (交织 I, Q, I, Q)
    float* data = nullptr;

    // 当前数据块中实际填充的采样点数 (通常等于配置中的 chunk_size)
    int num_samples = 0;

    // 辅助信息，方便调试和后续算法验证
    double current_carrier_phase = 0.0; // 本块起始时的载波相位
    double current_code_phase    = 0.0; // 本块起始时的码相位
};

} // namespace signal
} // namespace startrace

#endif // STARTRACE_SIGNAL_OUTPUT_H
