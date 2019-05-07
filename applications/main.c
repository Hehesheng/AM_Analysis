/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 * 2018-11-19     flybreak     add stm32f407-atk-explorer bsp
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

/* defined the LED0 pin: PF9 */
#define LED0_PIN    GET_PIN(F, 9)

static struct rt_memheap _ccm;
#define CCM_RAM_START (0x10000000)
#define CCM_RAM_SIZE (64 * 1024)

int main(void)
{
    int count = 1;
    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);

    while (count++)
    {
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }

    return RT_EOK;
}

/* init ccm ram */
int ccm_ram_init(void)
{
    for (int i = 0; i < CCM_RAM_SIZE / 4; i++)
    {
        *(uint32_t *)(CCM_RAM_START + i) = 0;
    }

    if (rt_memheap_init(&_ccm,
                        "ccm",
                        (void *)CCM_RAM_START,
                        CCM_RAM_SIZE) != RT_EOK)
    {
        rt_kprintf("ccm ram init fail\n");
        return RT_ERROR;
    }
    
    return RT_EOK;
}
INIT_BOARD_EXPORT(ccm_ram_init);
