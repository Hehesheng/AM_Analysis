#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include "drv_gpio.h"
#define LOG_TAG "dma_ad"  //该模块对应的标签。不定义时，默认：NO_TAG
#define LOG_LVL LOG_LVL_DBG  //该模块对应的日志输出级别。不定义时，默认：调试级别
#include <math.h>
#include <ulog.h>  //必须在 LOG_TAG 与 LOG_LVL 下面

#define ADC_INPUT_SIZE (3072U)
#define _TEST_BUF_SIZE (32U)
#define TEXT_BUF_SIZE (256U)
#define DEEPTH_OBJ_NAME "deep"
#define HMI_CMD_END "\xFF\xFF\xFF"

struct adc_res
{
    uint16_t min;
    uint16_t max;
    uint16_t midle;
    uint32_t sum;
    uint32_t ave;
};

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim8;

static rt_sem_t sem                           = RT_NULL;
static rt_sem_t out_switch                    = RT_NULL;
static int16_t adc_input_buff[ADC_INPUT_SIZE] = {0};
static uint8_t text_buf[TEXT_BUF_SIZE]        = {0};

extern void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{
    /* DMA controller clock enable */
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA2_Stream4_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void)
{
    /* USER CODE BEGIN ADC1_Init 0 */

    /* USER CODE END ADC1_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */

    /* USER CODE END ADC1_Init 1 */
    /**Configure the global features of the ADC (Clock, Resolution, Data
     * Alignment and number of conversion)
     */
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }
    /**Configure for the selected ADC regular channel its corresponding rank in
     * the sequencer and its sample time.
     */
    sConfig.Channel      = ADC_CHANNEL_1;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN ADC1_Init 2 */

    /* USER CODE END ADC1_Init 2 */
}

/*
 * @brief  冒泡排序
 * @param  [in]buf: 数组
 * #param  [in]len: 长度
 * @retval None
 */
void sort(uint16_t* buff, uint32_t size)
{
    int i, j;
    uint32_t t;

    if (size == 0) return;
    //冒泡排序
    for (i = 0; i < size - 1; i++)  // n个数的数列总共扫描n-1次
    {
        for (j = 0; j < size - i - 1; j++)
        {  //每一趟扫描到a[n-i-2]与a[n-i-1]比较为止结束
            if (buff[j] < buff[j + 1])
            {
                t           = buff[j + 1];
                buff[j + 1] = buff[j];
                buff[j]     = t;
            }
        }
    }
}

/**
 * @brief  找出数组中相关信息
 * @param  [in]buf: 数组
 * @param  [in]len: 长度
 * @param  [out]res: 结果
 * @return 调制深度
 */
static float find_the_result(uint16_t* buf, uint32_t len, struct adc_res* res)
{
    sort(buf, len);

    res->max   = buf[10];
    res->min   = buf[len - 1 - 10];
    res->midle = buf[len / 2];

    res->sum = 0;
    for (int i = 0; i < len; i++)
    {
        res->sum += buf[i];
    }
    res->ave = res->sum / len;

    return (res->midle - res->min) * 100.0 / (float)res->midle;
}

/**
 * @brief  fft计算线程
 * @param  None
 * @return None
 */
static void fft_thread(void* parameter)
{
    struct adc_res res;
    rt_device_t serial = RT_NULL;
    float deepth       = 0;

    /* 打开串口3 */
    serial = rt_device_find("uart3");
    if (serial != RT_NULL)
    {
        /* 以读写及中断接收方式打开串口设备 */
        rt_device_open(serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    }
    else
    {
        log_e("uart3 open fail");
    }

    while (1)
    {
        /* 等待DMA完成中断 */
        rt_sem_take(sem, RT_WAITING_FOREVER);

        deepth = find_the_result(adc_input_buff, ADC_INPUT_SIZE, &res);

        if (rt_sem_trytake(out_switch) == RT_EOK)
        {
            rt_kprintf(
                "===begin===\n"
                "res.min=%d,\n"
                "res.max=%d,\n"
                "res.midle=%d,\n"
                "res.sum=%d,\n"
                "res.ave=%d,\n",
                res.min, res.max, res.midle, res.sum, res.ave);
            rt_sem_release(out_switch);
        }

        rt_kprintf("=================================\n");
        if (deepth < 46 && deepth > 12)
        {
            deepth += 8;
        }
        else if (deepth <= 9)
        {
            deepth += 4;
        }
        else if (deepth > 65)
        {
            deepth -= 10;
        }
        
        rt_kprintf("running 222\n");
        
    
        rt_memset(text_buf, 0, TEXT_BUF_SIZE);
        sprintf(text_buf, DEEPTH_OBJ_NAME ".txt=\"%.3f\"" HMI_CMD_END, deepth);
        rt_device_write(serial, 0, text_buf, rt_strlen(text_buf));
        // rt_memset(text_buf, 0, TEXT_BUF_SIZE);
        sprintf(text_buf, "[(res->midle - res->min) / res->midle]deepth: %f %%\n", deepth);
        rt_kprintf("%s", text_buf);

        // deepth = (res.midle - res.min) * 100.0 / (float)res.ave;
        // sprintf(text_buf, "[(res.midle - res.min) / res.ave]deepth: %f %%\n", deepth);
        // rt_kprintf("%s", text_buf);

        // deepth = (res.ave - res.min) * 100.0 / (float)res.ave;
        // sprintf(text_buf, "[(res.ave - res.min) / res.ave]deepth: %f %%\n", deepth);
        // rt_kprintf("%s", text_buf);

        // deepth = (res.midle - (res.max - res.min) / 2) * 100.0 / (float)res.midle;
        // sprintf(text_buf, "[res.midle - (res.max - res.min) / 2) / res.midle]deepth: %f %%\n",
        //         deepth);
        // rt_kprintf("%s", text_buf);

        rt_memset(adc_input_buff, 0, ADC_INPUT_SIZE * sizeof(uint16_t));
        rt_thread_delay(RT_TICK_PER_SECOND / 2);
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_input_buff, ADC_INPUT_SIZE);
    }
}

/**
 * @brief  初始化adc_dma
 * @param  None
 * @return 0
 */
int adc_dma_init(void)
{
    MX_ADC1_Init();
    MX_DMA_Init();

    HAL_ADC_DeInit(&hadc1);
    HAL_DMA_DeInit(&hdma_adc1);

    MX_ADC1_Init();
    MX_DMA_Init();

    /* 创建dma通知信号量 */
    sem = rt_sem_create("sdma", 0, RT_IPC_FLAG_PRIO);
    if (sem == RT_NULL)
    {
        log_w("sem create fail!!");
    }
    /* 创建调制深度通知信号量 */
    out_switch = rt_sem_create("sout", 0, RT_IPC_FLAG_PRIO);
    if (out_switch == RT_NULL)
    {
        log_w("out_switch create fail!!");
    }
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_input_buff, ADC_INPUT_SIZE);

    rt_thread_t tid = rt_thread_create("tfft", fft_thread, RT_NULL, 8192, 25, 20);
    RT_ASSERT(tid != RT_NULL);
    rt_thread_startup(tid);

    // HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_input_buff, ADC_INPUT_SIZE);

    return 0;
}
INIT_ENV_EXPORT(adc_dma_init);

/**
 * @brief This function handles DMA2 stream4 global interrupt.
 */
void DMA2_Stream4_IRQHandler(void)
{
    /* USER CODE BEGIN DMA2_Stream4_IRQn 0 */
    rt_interrupt_enter();
    /* USER CODE END DMA2_Stream4_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_adc1);
    /* USER CODE BEGIN DMA2_Stream4_IRQn 1 */
    rt_interrupt_leave();
    /* USER CODE END DMA2_Stream4_IRQn 1 */
}

/**
 * @brief  DMA结束回调函数
 * @param  None
 * @return None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) { rt_sem_release(sem); }

/**
 * @brief  dma开始
 * @param  None
 * @return None
 */
static void dma_control(int argc, char** argv)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_input_buff, ADC_INPUT_SIZE);
}
MSH_CMD_EXPORT(dma_control, begin dma);

static void switch_out(int argc, char** argv)
{
    if (argc > 1)
    {
        if (rt_strcmp(argv[1], "on") == 0 && out_switch->value == 0)
        {
            rt_sem_release(out_switch);
        }
        else if (rt_strcmp(argv[1], "off") == 0 && out_switch->value == 1)
        {
            rt_sem_take(out_switch, RT_WAITING_FOREVER);
        }
        else
        {
            log_w("error cmd, ->value: %d", out_switch->value);
        }
    }
    else
    {
        log_w("error input, cmd like: switch_out <on|off>");
    }
}
MSH_CMD_EXPORT(switch_out, switch_out<on | off>);
