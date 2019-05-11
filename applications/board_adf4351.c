/*
 * ==============================
 * @File   board_adf4351.c
 * @Date   Created on 2018/11/16
 * @Author Hehesheng
 * @Other  频率计算公式有6个参数, WTF!!!
 * ==============================
 */
#include "board_adf4351.h"

#define FREQ_TO_MHZ(x) (x * 1000)
#define FREQ_TO_KHZ(x) (x)

// uint32_t adf_R        = 1;  // RF参考分频系数
// uint32_t adf_D        = 0;  // RF REFin倍频器位(0 or 1)
// uint32_t adf_T        = 0;  //参考二分频位,产生占空比50%,减少周跳
// uint32_t adf_Locktime = 8;
// uint32_t adf_MOD      = 4095;
// uint32_t adf_INT      = 256;
// uint32_t adf_FRAC     = 0;
// uint32_t adf_PHASE    = 1;
// uint32_t adf_DIV      = RF_div32;

// uint16_t adf_R        = 1;  // RF参考分频系数
// uint32_t adf_D         = 0;  // RF REFin倍频器位(0 or 1)
// uint32_t adf_T         = 0;  //参考二分频位,产生占空比50%,减少周跳
// uint16_t adf_Locktime = 160;
// uint16_t adf_MOD      = 1;
// uint16_t adf_INT      = 256;
// uint16_t adf_FRAC     = 0;
// uint16_t adf_PHASE    = 1;
// uint32_t adf_DIV       = RF_div32;

/**
 * @author    vfhky 2015.05.30 https://typecodes.com/cseries/simplifychexstrtoint.html
 * @param     [in]HexStr 十六进制字符串（例如"eE2"、"Fa1"、"2011"、"-eE2"、"+eE2"等）
 * @return    0:字符串转换成功; -1:字符串为空; -2:字符串中包含非十六进制的字符;
 */
int HexStr2Integer(char* HexStr, int* res)
{
    int iResult = 0, iFlag = 1;

    //判断字符串是否合法
    if (NULL == HexStr || (*HexStr == '+' && *(HexStr + 1) == '\0') ||
        (*HexStr == '-' && *(HexStr + 1) == '\0') ||
        (*HexStr == 0x30 && *(HexStr + 1) == 0x58 && *(HexStr + 2) == '\0') ||
        (*HexStr == 0x30 && *(HexStr + 1) == 0x78 && *(HexStr + 2) == '\0'))
    {
        return -1;
    }

    //指针变量p指向字符串的首位
    char* p = HexStr;

    //对以"+"、"-"号开头的十六进制字符串的处理
    if ('-' == *p || '+' == *p)
    {
        if (*p == '-') iFlag = -1;
        ++p;
    }
    //对"0x"或者"0X"开头的十六进制字符的处理
    else if ((*p == 0x30 && *(p + 1) == 0x58) || (*p == 0x30 && *(p + 1) == 0x78))
    {
        p += 2;
    }

    while (*p != '\0')
    {
        if (*p >= 48 && *p <= 57)
            iResult = (*p - 48) + (iResult << 4);
        else if (*p >= 65 && *p <= 70)
            iResult = (*p - 65 + 10) + (iResult << 4);
        else if (*p >= 97 && *p <= 102)
            iResult = (*p - 97 + 10) + (iResult << 4);
        else
            return -2;

        ++p;
    }
    *res = iFlag * iResult;

    return 0;
}

/**
 * @brief  adf4351使用的io初始化
 * @arg    None
 * @retval None
 */
static void adf4351_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOF_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(ADF4351_PORT, ADF4351_CLK_PIN | ADF4351_DATA_PIN | ADF4351_LE_PIN,
                      GPIO_PIN_RESET);

    GPIO_InitStruct.Pin   = ADF4351_CLK_PIN | ADF4351_DATA_PIN | ADF4351_LE_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(ADF4351_PORT, &GPIO_InitStruct);
}

/**
 * @brief  写一次数据
 * @arg    数据
 * @retval None
 */
static void adf4351_write(uint32_t data)
{
    uint8_t i;

    ADF4351_LE_RESET;  //开始传输数据

    for (i = 0; i < 32; i++)
    {
        ADF4351_CLK_RESET;
        if (data & 0x80000000)
        {
            ADF4351_DATA_SET;
        }
        else
        {
            ADF4351_DATA_RESET;
        }
        data <<= 1;
        ADF4351_CLK_SET;
    }
    ADF4351_CLK_RESET;
    ADF4351_LE_SET;  //传输完成
}

/**
 * @brief  初始化
 * @param  None
 * @return None
 */
int adf4351_init(void)
{
    uint32_t adf_data = 0;

    uint32_t adf_R        = 1;  // RF参考分频系数
    uint32_t adf_D        = 0;  // RF REFin倍频器位(0 or 1)
    uint32_t adf_T        = 0;  //参考二分频位,产生占空比50%,减少周跳
    uint32_t adf_Locktime = 8;
    uint32_t adf_MOD      = 4095;
    uint32_t adf_INT      = 256;
    uint32_t adf_FRAC     = 0;
    uint32_t adf_PHASE    = 1;
    uint32_t adf_DIV      = RF_div32;

    adf4351_gpio_init();

    //配置寄存器5
    adf_data = 0x00580000;  //数字锁存   LD引脚的工作方式为数字锁存   D23 D22=01
    adf_data = adf_data | ADF_R5;
    adf4351_write(adf_data);

    //配置寄存器4
    adf_data = 0x00800038;
    /*(DB23=1)The signal is taken from the VCO directly,信号直接从VCO获得
    可修改RF divider, R的值(DB22-20)the RF divider is 32;
    (DB11=0)VCO powerd up;	辅助RF输出禁用; 频段选择时钟,分频至125k,
    分频值160*/
    adf_data = adf_data | (adf_DIV << 20);  // RF divider is 32
    adf_data = adf_data | (160 << 12);      //频段选择时钟
    adf_data = adf_data | ADF_R4;           //(DB5=1)RF output is enabled;(DB4-3=3H)Output power
                                            // level is 5dBm
    adf4351_write(adf_data);

    //配置寄存器3
    adf_data = 0x00848000;
    // adf_data = 0x00000000;
    /*选择高频段（D23=1）, APB6ns(D22=0,=建议小数分频使用),,(D21=0,小数分频使用)
    使能CSR(D18=1),(D16 D15=01)快速锁定 可修改clock divider value的值*/
    adf_data = adf_data | (adf_Locktime << 3);
    adf_data = adf_data | ADF_R3;
    adf4351_write(adf_data);

    //配置寄存器2
    adf_data = 0x61002040;
    // adf_data = 0x00000E40;
    //低杂散输出, 禁用参考倍频, 二分频触发器使能(减少周跳必须)
    //使能双缓冲器, 设置电荷磅电流0.31, 小数N分频(40), 设置R分频器的值为1
    //设置鉴相器的极性, (DB6)同向滤波器1,反向滤波器0,这里用同向滤波器
    adf_data = adf_data | (adf_D << 25);
    adf_data = adf_data | (adf_T << 24);
    adf_data = adf_data | (adf_R << 14);
    adf_data = adf_data | ADF_R2;
    adf4351_write(adf_data);

    //配置寄存器1
    adf_data = 0x01008000;
    // adf_data = 0x08000000;
    //禁用相位调整,预分频器的值为8/9
    //相位字为1
    adf_data = adf_data | (adf_PHASE << 15);
    adf_data = adf_data | (adf_MOD << 3);
    adf_data = adf_data | ADF_R1;
    adf4351_write(adf_data);

    //配置寄存器0
    adf_data = 0x00000000;
    adf_data = adf_data | (adf_INT << 15);
    adf_data = adf_data | (adf_FRAC << 3);
    adf_data = adf_data | ADF_R0;
    adf4351_write(adf_data);

    log_i("ADF4351 init success");

    return 0;
}
INIT_APP_EXPORT(adf4351_init);

/**
 * @brief  获取分频系数
 * @param  freq:频率, 单位KHz;
 * @param  clock:时钟分频量
 * @return -1:参数有误; 0:参数正确
 */
static int adf4351_set_dive(uint32_t freq, struct adf4351_clock* clock)
{
    uint32_t f_PFD = ADF4351_F_PFD_FREQ / 1000;
    double compare = 0;

    if ((freq * 1000) < ADF4351_MIN_FRE || freq > ADF4351_MAX_FRE)
    {
        log_w("error freq input!");
        return -1;
    }

    {
        if (freq >= FREQ_TO_MHZ(2200))
        {
            clock->DIV = RF_div1;
        }
        else if (freq >= FREQ_TO_MHZ(1100))
        {
            clock->DIV = RF_div2;
        }
        else if (freq >= FREQ_TO_MHZ(550))
        {
            clock->DIV = RF_div4;
        }
        else if (freq >= FREQ_TO_MHZ(275))
        {
            clock->DIV = RF_div8;
        }
        else if (freq >= FREQ_TO_KHZ(137500))
        {
            clock->DIV = RF_div16;
        }
        else if (freq >= FREQ_TO_KHZ(68750))
        {
            clock->DIV = RF_div32;
        }
        else if (freq >= FREQ_TO_KHZ(34375))
        {
            clock->DIV = RF_div64;
        }
        else
        {
            log_w("error div search!!!");
            return -1;
        }
    }

    /* 获取需要配置的频率 */
    freq    = freq * (1 << clock->DIV);
    compare = (double)freq / (double)f_PFD;
    /* 得整数部分 */
    clock->INT = (uint16_t)compare;
    /* 小数部分 */
    clock->FRAC = (uint16_t)((compare - clock->INT) * 4096.0);
    // clock->MOD  = 4095;

    return 0;
}

/**
 * @brief  设置频率
 * @param  freq:频率,单位KHz
 * @return None
 */
void adf4351_set_freq(uint32_t freq)
{
    uint32_t adf_data = 0;

    uint32_t adf_R        = 1;  // RF参考分频系数
    uint32_t adf_D        = 0;  // RF REFin倍频器位(0 or 1)
    uint32_t adf_T        = 0;  //参考二分频位,产生占空比50%,减少周跳
    uint32_t adf_Locktime = 8;
    uint32_t adf_MOD      = 4095;
    uint32_t adf_INT      = 256;
    uint32_t adf_FRAC     = 0;
    uint32_t adf_PHASE    = 1;
    uint32_t adf_DIV      = RF_div32;

    struct adf4351_clock clock_cfg = {0};

    if (adf4351_set_dive(freq, &clock_cfg))
    {
        log_e("set freq fail!");
        return;
    }

    adf_DIV  = clock_cfg.DIV;
    adf_INT  = clock_cfg.INT;
    adf_FRAC = clock_cfg.FRAC;
    // adf_MOD  = clock_cfg.MOD;

    adf4351_gpio_init();

    //配置寄存器5
    adf_data = 0x00580000;  //数字锁存   LD引脚的工作方式为数字锁存   D23 D22=01
    adf_data = adf_data | ADF_R5;
    adf4351_write(adf_data);

    //配置寄存器4
    adf_data = 0x00800038;
    /*(DB23=1)The signal is taken from the VCO directly,信号直接从VCO获得
    可修改RF divider, R的值(DB22-20)the RF divider is 32;
    (DB11=0)VCO powerd up;	辅助RF输出禁用; 频段选择时钟,分频至125k,
    分频值160*/
    adf_data = adf_data | (adf_DIV << 20);  // RF divider is 32
    adf_data = adf_data | (160 << 12);      //频段选择时钟
    adf_data = adf_data | ADF_R4;           //(DB5=1)RF output is enabled;(DB4-3=3H)Output power
                                            // level is 5dBm
    adf4351_write(adf_data);

    //配置寄存器3
    adf_data = 0x00848000;
    // adf_data = 0x00000000;
    /*选择高频段（D23=1）, APB6ns(D22=0,=建议小数分频使用),,(D21=0,小数分频使用)
    使能CSR(D18=1),(D16 D15=01)快速锁定 可修改clock divider value的值*/
    adf_data = adf_data | (adf_Locktime << 3);
    adf_data = adf_data | ADF_R3;
    adf4351_write(adf_data);

    //配置寄存器2
    adf_data = 0x61002040;
    // adf_data = 0x00000E40;
    //低杂散输出, 禁用参考倍频, 二分频触发器使能(减少周跳必须)
    //使能双缓冲器, 设置电荷磅电流0.31, 小数N分频(40), 设置R分频器的值为1
    //设置鉴相器的极性, (DB6)同向滤波器1,反向滤波器0,这里用同向滤波器
    adf_data = adf_data | (adf_D << 25);
    adf_data = adf_data | (adf_T << 24);
    adf_data = adf_data | (adf_R << 14);
    adf_data = adf_data | ADF_R2;
    adf4351_write(adf_data);

    //配置寄存器1
    adf_data = 0x01008000;
    // adf_data = 0x08000000;
    //禁用相位调整,预分频器的值为8/9
    //相位字为1
    adf_data = adf_data | (adf_PHASE << 15);
    adf_data = adf_data | (adf_MOD << 3);
    adf_data = adf_data | ADF_R1;
    adf4351_write(adf_data);

    //配置寄存器0
    adf_data = 0x00000000;
    adf_data = adf_data | (adf_INT << 15);
    adf_data = adf_data | (adf_FRAC << 3);
    adf_data = adf_data | ADF_R0;
    adf4351_write(adf_data);
}

#include <stdlib.h>
static void adf4351_test(int argc, char** argv)
{
    if (argc > 1)
    {
        uint32_t freq = atoi(argv[1]);
        log_d("input freq: %d MHz", freq);
        // freq *= 1000;
        adf4351_set_freq(freq * 1000);
    }
    else
    {
        log_w("lost argv, like \"adf_freq 2000 \"");
    }
}
MSH_CMD_EXPORT_ALIAS(adf4351_test, adf_freq, freq Mhz : adf_freq<freq>);

static void adf4351_init_test(void) { adf4351_init(); }
MSH_CMD_EXPORT_ALIAS(adf4351_init_test, adf_init, testesetss);
