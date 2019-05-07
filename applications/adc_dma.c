#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include "drv_gpio.h"
#define LOG_TAG "dma_ad"  //该模块对应的标签。不定义时，默认：NO_TAG
#define LOG_LVL LOG_LVL_DBG  //该模块对应的日志输出级别。不定义时，默认：调试级别
#include <math.h>
#include <ulog.h>  //必须在 LOG_TAG 与 LOG_LVL 下面

#include "app_fun.h"
#include "arm_math.h"

#define ADC_INPUT_SIZE (8192U)
#define _TEST_BUFF_SIZE (32U)

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim8;

static rt_sem_t sem                           = RT_NULL;
static int16_t adc_input_buff[ADC_INPUT_SIZE] = {0};

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
    sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN ADC1_Init 2 */

    /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief TIM8 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM8_Init(void)
{
    /* USER CODE BEGIN TIM8_Init 0 */

    /* USER CODE END TIM8_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig               = {0};
    TIM_OC_InitTypeDef sConfigOC                        = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    /* USER CODE BEGIN TIM8_Init 1 */

    /* USER CODE END TIM8_Init 1 */
    htim8.Instance               = TIM8;
    htim8.Init.Prescaler         = 83;
    htim8.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim8.Init.Period            = 1999;
    htim8.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim8.Init.RepetitionCounter = 0;
    if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 999;
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }
    sBreakDeadTimeConfig.OffStateRunMode  = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel        = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime         = 0;
    sBreakDeadTimeConfig.BreakState       = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM8_Init 2 */

    /* USER CODE END TIM8_Init 2 */
    HAL_TIM_MspPostInit(&htim8);
}

/**
 * @brief  fft计算线程
 * @param  None
 * @return None
 */
static void fft_thread(void* parameter)
{
    /* fft初始化信息 */
    arm_rfft_instance_q15 S;
    /* CCM内存池 */
    struct rt_memheap* ccm_space =
        (struct rt_memheap*)rt_object_find("ccm", RT_Object_Class_MemHeap);
    /* sdma信号量 */
    // rt_sem_t sem           = (rt_sem_t)rt_object_find("sdma", RT_Object_Class_Semaphore);
    void* output_pool      = RT_NULL;
    q15_t* int_point       = RT_NULL;
    float32_t* float_point = RT_NULL;

    /* 变量检查 */
    // RT_ASSERT(sem != RT_NULL);
    RT_ASSERT(ccm_space != RT_NULL)
    /* 申请内存 */
    output_pool = (void*)rt_memheap_alloc(ccm_space, ADC_INPUT_SIZE * sizeof(q15_t) * 2);
    RT_ASSERT(output_pool != RT_NULL);
    /* int_point 指向内存池头 类型用于处理整数
       float_point 指向内存池头 类型用于浮点数处理*/
    int_point   = (q15_t*)output_pool;
    float_point = (float32_t*)output_pool;

    while (1)
    {
        /* 等待DMA完成中断 */
        rt_sem_take(sem, RT_WAITING_FOREVER);
        /* 初始变换且检查 */
        RT_ASSERT(arm_rfft_init_q15(&S, ADC_INPUT_SIZE, 0, 1) == ARM_MATH_SUCCESS);
        arm_rfft_q15(&S, (q15_t*)adc_input_buff, int_point);
        /* 结果取模并转为小数 */
        for (uint32_t i = 0; i < ADC_INPUT_SIZE; i++)
        {
            double tmp = 0;
            tmp = pow((double)*(int_point + 2 * i), 2) + pow((double)*(int_point + 2 * i + 1), 2);
            *(float_point + i) = (float32_t)sqrt(tmp);
        }

        for (uint32_t i = 0; i < ADC_INPUT_SIZE / 2; i++)
        {
            if (*(float_point + i) > 20)
            {
                rt_kprintf("[%4d]: %d\n", i, (int32_t) * (float_point + i));
            }
        }
        rt_thread_delay(RT_TICK_PER_SECOND);
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
    /* 产生PWM */
    MX_TIM8_Init();
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);

    /* 创建dma通知信号量 */
    sem = rt_sem_create("sdma", 0, RT_IPC_FLAG_PRIO);
    if (sem == RT_NULL)
    {
        log_w("sem create fail!!");
    }

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
