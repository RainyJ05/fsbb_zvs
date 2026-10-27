//#############################################################################
//
// FILE:   empty_driverlib_main.c
//
//! \addtogroup driver_example_list
//! <h1>Empty Project Example</h1> 
//!
//! This example is an empty project setup for Driverlib development.
//!
//
//#############################################################################
//
//
// $Copyright:
// Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000ware_libraries.h"
#include "zvs_t2_lut.h"
#include "DCL.h"
#include "DCLF32.h"
#include <math.h>
#include "vofa.h"
#include "key.h"


// 全局变量
uint16_t data_read[7] = {0};    // ADC原始值

float32_t T1 = 0.0f;
float32_t T2 = 0.0f;
float32_t T3 = 0.0f;

DCL_PI Iout_controller = PI_DEFAULTS; // PI控制参考电流
float32_t I_Kp = 0.5f;
float32_t I_Ki = 0.0001f;
float32_t I_Umax = 15.0f;
float32_t I_Umin = 0.0f;

struct
{
    volatile uint8_t error;// 1 - 故障 0 - 正常
    volatile uint8_t run;  // 0 - 停止 1 - 运行
} flags = {0};           // 标志位结构体


struct
{
    float32_t Vin;
    float32_t Vout;
    float32_t Iin;
    float32_t Iout;
    float32_t IL;
} data_conv = {0.0f};           // 实际物理量

float32_t Iout_ref = 0.0f;      // 输出参考电流
float32_t Vout_ref = 0.0f;      // 输出参考电压

//
// Main
//
void main(void)
{

    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pull-ups.
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    // 关时钟
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // PinMux and Peripheral Initialization
    //
    Board_init();

    // 开时钟
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // C2000Ware Library initialization
    //
    C2000Ware_libraries_init();

    VOFA_Init();

    // 写入控制器参数
    Iout_controller.Kp = I_Kp;
    Iout_controller.Ki = I_Ki;
    Iout_controller.Umax = I_Umax;
    Iout_controller.Umin = I_Umin;

    // 开启串口调试接口，可在线修改输出参考电压和 PI 参数
    VOFA_DebugInit(&Vout_ref, &Iout_controller.Kp, &Iout_controller.Ki);

    //
    // Enable Global Interrupt (INTM) and real time interrupt (DBGM)
    //
    EINT;
    ERTM;

    // 关输出
    EPWM_forceTripZoneEvent(myEPWM1_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(myEPWM2_BASE, EPWM_TZ_FORCE_EVENT_OST);

    // 关指示灯
    GPIO_writePin(LED1, 1);
    GPIO_writePin(LED2, 1);
    // 开按键扫描
    Key_Open();

    uint8_t key_num = 0;

    while(1)
    {
        key_num = Key_GetNum();
        if(key_num == 1)
        {
            if(flags.run == 1)
            {
                // 关输出
                EPWM_forceTripZoneEvent(myEPWM1_BASE, EPWM_TZ_FORCE_EVENT_OST);
                EPWM_forceTripZoneEvent(myEPWM2_BASE, EPWM_TZ_FORCE_EVENT_OST);

                // 关指示灯
                GPIO_writePin(LED1, 1);

                // 重置控制器
                DCL_resetPI(&Iout_controller);

                // 清零中间变量
                Iout_ref = 0;

                // 置位
                flags.run = 0;
            }
            else
            {
                // 检查故障
                if(flags.error == 0)
                {
                    // 释放
                    EPWM_clearTripZoneFlag(myEPWM1_BASE, EPWM_TZ_FLAG_OST);
                    EPWM_clearTripZoneFlag(myEPWM2_BASE, EPWM_TZ_FLAG_OST);

                    // 开指示灯
                    GPIO_writePin(LED1, 0);

                    // 置位
                    flags.run = 1;
                }
            }
        }

        VOFA_SetChannel(0, data_conv.Vout);
        VOFA_SetChannel(1, data_conv.Iout);
        VOFA_SetChannel(2, Vout_ref);
        VOFA_SetChannel(3, Iout_ref);

        VOFA_Send();
    }
}

// ADCC转换完成中断，RAM加速
#pragma CODE_SECTION(INT_myADCC_1_ISR, ".TI.ramfunc")
interrupt void INT_myADCC_1_ISR(void)
{
//    GPIO_writePin(28, 1);

//    data_read[0] = HWREGH(ADCARESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER0);
//    data_read[1] = HWREGH(ADCARESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER1);
    data_read[2] = HWREGH(ADCBRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER0);
    data_read[3] = HWREGH(ADCBRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER1);
    data_read[4] = HWREGH(ADCCRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER0);
    data_read[5] = HWREGH(ADCCRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER1);
    data_read[6] = HWREGH(ADCCRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2);

    data_conv.Vin = data_read[2] * 0.0258f + 0.0853f;
    data_conv.Vout = data_read[3] * 0.0259f + 0.1504f;
    data_conv.Iin = ((float32_t)data_read[4] - (float32_t)data_read[5]) * 0.0081f - 0.1257f;
    data_conv.Iout = ((float32_t)data_read[6] - (float32_t)data_read[5]) * 0.0082f - 0.0127f;

    // 保护
    if(data_conv.Vout > 80.0 || data_conv.Iout > 8.0)
    {
        // 跳闸
        EPWM_forceTripZoneEvent(myEPWM1_BASE, EPWM_TZ_FORCE_EVENT_OST);
        EPWM_forceTripZoneEvent(myEPWM2_BASE, EPWM_TZ_FORCE_EVENT_OST);

        // 置位
        flags.error = 1;
        flags.run = 0;

        // 关指示灯
        GPIO_writePin(LED1, 1);

        // 重置控制器
        DCL_resetPI(&Iout_controller);

        // 清零中间变量
        Iout_ref = 0;

        // 跳过控制
        goto error_return;
    }
    else
    {
        flags.error = 0;
    }

    if(flags.run == 1 && flags.error == 0)
    {
        // PI生成Iout_ref
        Iout_ref = DCL_runPI_C3(&Iout_controller, Vout_ref, data_conv.Vout);

        // T2查表：Vo使用本次ADC采样值，T2为归一化时间 T2/Ts。
        T2 = ZVS_T2_Lookup(data_conv.Vin, data_conv.Vout, Iout_ref);

        // 在归一化时间域计算T1、T3，避免ISR内反复进行Ts的乘除。
        if (T2 < 0.0F)
        {
            // 查表无效时，禁止负的T2进入PWM比较值计算。
            T1 = 0.0F;
            T2 = 0.0F;
            T3 = 0.0F;
        }
        else
        {
            const float32_t l_over_ts = ZVS_T2_INDUCTANCE_H / ZVS_T2_SWITCH_PERIOD_S;
            const float32_t i0_l_over_ts = ZVS_T2_INITIAL_CURRENT_A * l_over_ts;
            const float32_t disc = i0_l_over_ts * i0_l_over_ts + 2.0F * Iout_ref * data_conv.Vout * l_over_ts + data_conv.Vin * data_conv.Vout * T2 * T2;

            if (disc < 0.0F)
            {
                // 插值造成无效判别式时，使用零时间值。
                T1 = 0.0F;
                T2 = 0.0F;
                T3 = 0.0F;
            }
            else
            {
                T1 = (i0_l_over_ts - data_conv.Vin * T2 + sqrtf(disc)) / data_conv.Vin;
                T3 = (data_conv.Vin / data_conv.Vout) * (T1 + T2) - T2;

                // 防止插值误差导致负时间。
                if (T1 < 0.0F) T1 = 0.0F;
                if (T3 < 0.0F) T3 = 0.0F;
            }
        }

        // 更新占空比
        EPWM_setCounterCompareValue(myEPWM1_BASE, EPWM_COUNTER_COMPARE_A, (T1 + T2) * 250);
        EPWM_setCounterCompareValue(myEPWM2_BASE, EPWM_COUNTER_COMPARE_A, T1 * 250);
        EPWM_setCounterCompareValue(myEPWM2_BASE, EPWM_COUNTER_COMPARE_B, (T1 + T2 + T3) * 250);
    }

error_return:

    // 清除中断标志位
    ADC_clearInterruptStatus(myADCC_BASE, ADC_INT_NUMBER1);

    // 检查溢出标志
    if (true == ADC_getInterruptOverflowStatus(myADCC_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(myADCC_BASE, ADC_INT_NUMBER1);
        ADC_clearInterruptStatus(myADCC_BASE, ADC_INT_NUMBER1);
    }

    // 应答中断相量
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);

//    GPIO_writePin(28, 0);
}

// 1kHz定时器中断
__interrupt void INT_myCPUTIMER0_ISR(void)
{

//    GPIO_writePin(28, 1);

    Key_Tick();
    VOFA_DebugPoll();

    // 清除中断标志位
    Interrupt_clearACKGroup(INT_myCPUTIMER0_INTERRUPT_ACK_GROUP);

//    GPIO_writePin(28, 0);
}


//
// End of File
//
