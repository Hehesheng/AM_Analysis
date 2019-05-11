/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2019-04-14     Hehesheng         first version
 */

#include <board.h>
#include "drv_sram.h"

#define LOG_TAG "drv.sram"
#include <drv_log.h>

#ifdef SOC_SERIES_STM32F4
#ifdef BSP_USING_EXT_FMC_IO

static struct rt_memheap _sram;

static int try_sram(void)
{
    uint32_t *p = RT_NULL;

    p = (uint32_t *)SRAM_ADDR_BASE;

    for (int i = 0;; i++)
    {
        *p = 0xFFFF0000;
        if (0xFFFF0000 != *p)
        {
            LOG_E("sram error: %p", p);
            return -1;
        }
        p++;
        if ((SRAM_ADDR_BASE + SRAM_SIZE / 4) == (uint32_t)p)
            break;
    }
    LOG_I("sram ok");

    return 0;
}

static void sram_fill_zero(void)
{
    for (int i = 0; i < SRAM_SIZE / 4; i++)
    {
        *(uint32_t *)(SRAM_ADDR_BASE + i) = 0;
    }
}

/* SRAM initialization function */
static int hw_sram_init(void)
{
    SRAM_HandleTypeDef hsram1;
    FSMC_NORSRAM_TimingTypeDef Timing;
    /** Perform the SRAM1 memory initialization sequence
     */
    hsram1.Instance = FSMC_NORSRAM_DEVICE;
    hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    /* hsram1.Init */
    hsram1.Init.NSBank = FSMC_NORSRAM_BANK3;
    hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
    hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    hsram1.Init.PageSize = FSMC_PAGE_SIZE_NONE;
    /* Timing */
    Timing.AddressSetupTime = 0;
    Timing.AddressHoldTime = 15;
    Timing.DataSetupTime = 10;
    Timing.BusTurnAroundDuration = 0;
    Timing.CLKDivision = 16;
    Timing.DataLatency = 17;
    Timing.AccessMode = FSMC_ACCESS_MODE_A;
    /* ExtTiming */

    HAL_SRAM_MspInit(RT_NULL);
    HAL_SRAM_Init(&hsram1, &Timing, &Timing);

    if (try_sram())
    {
        rt_kprintf("sram init fail\n");
        return RT_ERROR;
    }
    sram_fill_zero();

    if (rt_memheap_init(&_sram,
                        "sram",
                        (void *)SRAM_ADDR_BASE,
                        SRAM_SIZE) != RT_EOK)
    {
        rt_kprintf("sram init fail\n");
        return RT_ERROR;
    }

    return RT_EOK;
}
// INIT_BOARD_EXPORT(hw_sram_init);

#ifdef SRAM_DEBUG
#ifdef FINSH_USING_MSH

void sram_test(void)
{
    uint32_t *p = RT_NULL;

    p = (uint32_t *)SRAM_ADDR_BASE;

    for (int i = 0;; i++)
    {
        *p = 0xFFFF0000;
        if (0xFFFF0000 != *p)
        {
            LOG_E("sram error: %p", p);
            return;
        }
        p++;
        if ((SRAM_ADDR_BASE + SRAM_SIZE) == (uint32_t)p)
            break;
    }
    LOG_I("sram ok");

    return;
}
MSH_CMD_EXPORT(sram_test, sram test);

void sram_write(int argc, char **argv)
{
    uint32_t offset = 0, data = 0;
    uint32_t *p = (uint32_t *)SRAM_ADDR_BASE;

    if (argc > 2)
    {
        offset = atoi(argv[1]);
        data = atoi(argv[2]);
        *(p + offset) = data;
        rt_kprintf("write %d at %p\n", data, p + offset);
    }
}
MSH_CMD_EXPORT(sram_write, write<offset><data>);

void sram_read(int argc, char **argv)
{
    uint32_t offset = 0, data = 0;
    uint32_t *p = (uint32_t *)SRAM_ADDR_BASE;

    if (argc > 1)
    {
        offset = atoi(argv[1]);
        data = atoi(argv[2]);
        data = *(p + offset);
        rt_kprintf("read %d at %p\n", data, p + offset);
    }
}
MSH_CMD_EXPORT(sram_read, read<offset>);

#endif /* FINSH_USING_MSH */
#endif /* SRAM_DEBUG */

#endif /* BSP_USING_EXT_FMC_IO */
#endif /* SOC_SERIES_STM32F4 */
