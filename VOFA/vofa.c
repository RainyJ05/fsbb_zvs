#include "vofa.h"

VOFA_Frame_t vofa_frame;

static char vofa_debug_line[VOFA_DEBUG_LINE_SIZE];
static uint16_t vofa_debug_line_length;
static uint8_t vofa_debug_line_overflow;
static float *vofa_debug_voltage_reference;
static float *vofa_debug_pi_kp;
static float *vofa_debug_pi_ki;

// VOFA初始化函数
void VOFA_Init(void)
{
    // 初始化帧尾: 0x00, 0x00, 0x80, 0x7F
    // C28x 内存布局 (小端模式处理):
    vofa_frame.tail[0] = 0x0000; // 对应前两个字节 0x00, 0x00
    vofa_frame.tail[1] = 0x7F80; // 对应后两个字节 0x80, 0x7F (注意高低位)

    // 清零所有通道
    uint16_t i;
    for (i = 0; i < VOFA_CHANNELS; i++)
    {
        vofa_frame.fdata[i] = 0.0f;
    }
}

// 初始化串口调试接口
void VOFA_DebugInit(float *voltage_reference, float *pi_kp, float *pi_ki)
{
    vofa_debug_voltage_reference = voltage_reference;
    vofa_debug_pi_kp = pi_kp;
    vofa_debug_pi_ki = pi_ki;
    vofa_debug_line_length = 0U;
    vofa_debug_line_overflow = 0U;
}

// 设置单个通道数据
void VOFA_SetChannel(uint16_t channel, float value)
{
    if (channel < VOFA_CHANNELS)
    {
        vofa_frame.fdata[channel] = value;
    }
}

// 设置所有通道数据
void VOFA_SetAllChannels(float values[])
{
    uint16_t i;
    for (i = 0; i < VOFA_CHANNELS; i++)
    {
        vofa_frame.fdata[i] = values[i];
    }
}

// 查找命令参数起始位置，命令与参数之间允许使用空格或制表符
static const char *VOFA_DebugCommandArgs(const char *line, const char *command)
{
    while ((*line != '\0') && (*command != '\0') && (*line == *command))
    {
        line++;
        command++;
    }

    if (*command != '\0') return (const char *)0;
    if ((*line != '\0') && (*line != ' ') && (*line != '\t'))
        return (const char *)0;

    while ((*line == ' ') || (*line == '\t')) line++;
    return line;
}

// 解析一个十进制浮点数，不依赖标准库浮点转换函数
static uint8_t VOFA_DebugParseFloat(const char **cursor, float *value)
{
    const char *p = *cursor;
    float result = 0.0f;
    float fraction = 0.1f;
    float sign = 1.0f;
    uint16_t digit_count = 0U;

    while ((*p == ' ') || (*p == '\t')) p++;

    if (*p == '-')
    {
        sign = -1.0f;
        p++;
    }
    else if (*p == '+')
    {
        p++;
    }

    while ((*p >= '0') && (*p <= '9'))
    {
        result = result * 10.0f + (float)(*p - '0');
        digit_count++;
        p++;
    }

    if (*p == '.')
    {
        p++;
        while ((*p >= '0') && (*p <= '9'))
        {
            result += (float)(*p - '0') * fraction;
            fraction *= 0.1f;
            digit_count++;
            p++;
        }
    }

    if (digit_count == 0U) return 0U;

    *value = sign * result;
    *cursor = p;
    return 1U;
}

// 检查浮点数后是否只剩空白字符
static uint8_t VOFA_DebugOnlyTrailingSpace(const char *cursor)
{
    while ((*cursor == ' ') || (*cursor == '\t')) cursor++;
    return (*cursor == '\0') ? 1U : 0U;
}

// 解析只包含一个浮点参数的命令
static uint8_t VOFA_DebugParseSingleFloat(const char *args, float *value)
{
    if (VOFA_DebugParseFloat(&args, value) == 0U) return 0U;
    return VOFA_DebugOnlyTrailingSpace(args);
}

// 应用一行串口调试命令
static void VOFA_DebugApplyLine(void)
{
    const char *args;
    float value;

    vofa_debug_line[vofa_debug_line_length] = '\0';

    args = VOFA_DebugCommandArgs(vofa_debug_line, "vref");
    if ((args != (const char *)0) &&
        (VOFA_DebugParseSingleFloat(args, &value) != 0U) &&
        (value >= 0.0f) && (value <= VOFA_DEBUG_VREF_MAX) &&
        (vofa_debug_voltage_reference != (float *)0))
    {
        *vofa_debug_voltage_reference = value;
        return;
    }

    args = VOFA_DebugCommandArgs(vofa_debug_line, "ikp");
    if ((args != (const char *)0) &&
        (VOFA_DebugParseSingleFloat(args, &value) != 0U) &&
        (value >= 0.0f) && (value <= VOFA_DEBUG_PI_KP_MAX) &&
        (vofa_debug_pi_kp != (float *)0))
    {
        *vofa_debug_pi_kp = value;
        return;
    }

    args = VOFA_DebugCommandArgs(vofa_debug_line, "iki");
    if ((args != (const char *)0) &&
        (VOFA_DebugParseSingleFloat(args, &value) != 0U) &&
        (value >= 0.0f) && (value <= VOFA_DEBUG_PI_KI_MAX) &&
        (vofa_debug_pi_ki != (float *)0))
    {
        *vofa_debug_pi_ki = value;
    }
}

// 非阻塞轮询串口调试命令，命令以回车或换行结束
void VOFA_DebugPoll(void)
{
    uint16_t received;

    while (SCI_getRxFIFOStatus(VOFA_SCI_BASE) != SCI_FIFO_RX0)
    {
        received = SCI_readCharNonBlocking(VOFA_SCI_BASE) & 0x00FFU;

        if ((received == '\r') || (received == '\n'))
        {
            if ((vofa_debug_line_overflow == 0U) &&
                (vofa_debug_line_length != 0U))
            {
                VOFA_DebugApplyLine();
            }

            vofa_debug_line_length = 0U;
            vofa_debug_line_overflow = 0U;
        }
        else if (received == '\b')
        {
            if (vofa_debug_line_length != 0U)
            {
                vofa_debug_line_length--;
            }
        }
        else if (vofa_debug_line_overflow == 0U)
        {
            if (vofa_debug_line_length < (VOFA_DEBUG_LINE_SIZE - 1U))
            {
                vofa_debug_line[vofa_debug_line_length++] = (char)received;
            }
            else
            {
                vofa_debug_line_overflow = 1U;
            }
        }
    }
}

/**
 * 辅助函数：通过 SCI 发送一个字节 (8-bit)
 */
static void SCI_SendByte(uint16_t b)
{
//    while (SciaRegs.SCIFFTX.bit.TXFFST != 0); // 等待发送 FIFO 有空间
//    SciaRegs.SCITXBUF.all = (b & 0xFF);       // 只取低 8 位
    SCI_writeCharBlockingFIFO(VOFA_SCI_BASE, b);
}

// 发送数据到VOFA+ (JustFloat 协议: 数据 + 0x00 0x00 0x80 0x7F)
void VOFA_Send(void)
{
    uint16_t i;
    uint32_t *pRawData;
    uint32_t temp;

    // 1. 发送浮点数据
    for (i = 0; i < VOFA_CHANNELS; i++)
    {
        // 将 float 指针转为 uint32 指针以进行位操作
        pRawData = (uint32_t *)&vofa_frame.fdata[i];
        temp = *pRawData;

        // C28x 发送必须拆分为 4 个 8-bit 字节
        SCI_SendByte(temp & 0xFF);         // Byte 0
        SCI_SendByte((temp >> 8) & 0xFF);  // Byte 1
        SCI_SendByte((temp >> 16) & 0xFF); // Byte 2
        SCI_SendByte((temp >> 24) & 0xFF); // Byte 3
    }

    // 2. 发送帧尾 (0x00, 0x00, 0x80, 0x7F)
    SCI_SendByte(0x00);
    SCI_SendByte(0x00);
    SCI_SendByte(0x80);
    SCI_SendByte(0x7F);
}

