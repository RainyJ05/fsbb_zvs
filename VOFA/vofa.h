#ifndef _VOFA_H_
#define _VOFA_H_

#include "board.h"
#include <stdint.h>

// 配置区域 ------------------------------------------------------
#define VOFA_CHANNELS 4  // 默认通道数
#define VOFA_SCI_BASE SCIA_BASE // 替换为实际使用的串口

/* 调试串口接收缓冲区和参数范围。 */
#define VOFA_DEBUG_LINE_SIZE 48U
#define VOFA_DEBUG_VREF_MAX 70.0f
#define VOFA_DEBUG_PI_KP_MAX 100.0f
#define VOFA_DEBUG_PI_KI_MAX 10.0f


// C28x 中 float 是 32-bit，对应 2 个 16-bit word
// 帧尾由 4 个 8-bit 字节组成 (0x00 0x00 0x80 0x7F)
// 在 C28x 中，这占用 2 个 16-bit word
typedef struct
{
    float fdata[VOFA_CHANNELS]; // 32-bit float
    uint16_t tail[2];           // 存储 4 个字节的帧尾
} VOFA_Frame_t;

extern VOFA_Frame_t vofa_frame;

// 初始化VOFA
void VOFA_Init(void);

// 设置单个通道数据
void VOFA_SetChannel(uint16_t channel, float value);

// 设置所有通道数据
void VOFA_SetAllChannels(float values[]);

// 发送数据到VOFA+ (针对 SCI 串口)
void VOFA_Send(void);

// 初始化串口调试接口，绑定输出参考电压和 PI 参数
void VOFA_DebugInit(float *voltage_reference, float *pi_kp, float *pi_ki);

// 命令格式：vref <V>、ikp <Kp>、iki <Ki>，以回车或换行结束
// 参数格式错误或超出范围时忽略；SCIA 同时发送 VOFA 二进制遥测，不回传文本
void VOFA_DebugPoll(void);

#endif
