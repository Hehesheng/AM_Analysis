#ifndef __BOARD_ADF4351_H__
#define __BOARD_ADF4351_H__
/*
 * ==============================
 * @File   board_adf435.h
 * @Date   Created on 2018/11/16
 * @Author Hehesheng
 * ==============================
 */

#include <rtthread.h>
#include <stm32f4xx.h>

#ifdef RT_USING_ULOG
#define LOG_TAG "adf4351"  //该模块对应的标签。不定义时，默认：NO_TAG
#define LOG_LVL LOG_LVL_DBG  //该模块对应的日志输出级别。不定义时，默认：调试级别
#include <ulog.h>            //必须在 LOG_TAG 与 LOG_LVL 下面
#endif                       /* RT_USING_ULOG */

// Hardware Option
#define ADF4351_PORT GPIOE
#define ADF4351_CLK_PIN GPIO_PIN_7
#define ADF4351_DATA_PIN GPIO_PIN_8
#define ADF4351_LE_PIN GPIO_PIN_9

#define ADF4351_CLK_SET HAL_GPIO_WritePin(ADF4351_PORT, ADF4351_CLK_PIN, GPIO_PIN_SET)
#define ADF4351_CLK_RESET HAL_GPIO_WritePin(ADF4351_PORT, ADF4351_CLK_PIN, GPIO_PIN_RESET)
#define ADF4351_DATA_SET HAL_GPIO_WritePin(ADF4351_PORT, ADF4351_DATA_PIN, GPIO_PIN_SET)
#define ADF4351_DATA_RESET HAL_GPIO_WritePin(ADF4351_PORT, ADF4351_DATA_PIN, GPIO_PIN_RESET)
#define ADF4351_LE_SET HAL_GPIO_WritePin(ADF4351_PORT, ADF4351_LE_PIN, GPIO_PIN_SET)
#define ADF4351_LE_RESET HAL_GPIO_WritePin(ADF4351_PORT, ADF4351_LE_PIN, GPIO_PIN_RESET)

#define ADF4351_REFIN_FRE (25000000U)
#define ADF4351_F_PFD_FREQ (12500000U)
#define ADF4351_MAX_FRE (4400000000U)
#define ADF4351_MIN_FRE (35000000U)

#define ADF_R0 (0)
#define ADF_R1 (1)
#define ADF_R2 (2)
#define ADF_R3 (3)
#define ADF_R4 (4)
#define ADF_R5 (5)

#define RF_div1 (0)
#define RF_div2 (1)
#define RF_div4 (2)
#define RF_div8 (3)
#define RF_div16 (4)
#define RF_div32 (5)
#define RF_div64 (6)

/* RF_OUT = [INT + (FRAC/MOD)] × (f_PFD / DIV) */
/* f_PFD = REF_IN × [(1 + D)/(R × (1 + T))] */
/* RF_OUT = [INT + (FRAC/MOD)] × (REF_IN × [(1 + D)/(R × (1 + T))] / DIV) */
/* REF_IN = 25M, R = 1, D = 0, T = 0 -> f_PFD = 12.5M */
struct adf4351_clock
{
    uint16_t R;     // [0:9] RF参考分频系数 (1)
    uint8_t D;      // (0 or 1) RF REFin倍频器位 (0)
    uint8_t T;      // (0 or 1) 参考二分频位,产生占空比50%,减少周跳 (0)
    uint8_t DIV;    // [0-6] (32)
    uint16_t INT;   // [0:15] (256)
    uint16_t FRAC;  // [0:9] (0)
    uint16_t MOD;   // [0:11] (1)
    // uint16_t Locktime;
};

int ADF4351_Init(void);
void adf4351_set_freq(uint32_t freq);

#endif  // __BOARD_ADF4351_H__
