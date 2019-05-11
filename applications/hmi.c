#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include "drv_gpio.h"
#define LOG_TAG "hmi"        //该模块对应的标签。不定义时，默认：NO_TAG
#define LOG_LVL LOG_LVL_DBG  //该模块对应的日志输出级别。不定义时，默认：调试级别
#include <hmi_cmd.h>
#include <stdlib.h>
#include <ulog.h>  //必须在 LOG_TAG 与 LOG_LVL 下面

#define HMI_THREAD_STACK_SIZE (2048)
#define HMI_THREAD_PRIORITY (20)
#define RX_BUFF_SIZE (30)

static rt_sem_t hmi_sem = RT_NULL;

void hmi_thread(void *parameter)
{
    char rx_buff[RX_BUFF_SIZE] = {0};
    char tmp                   = 0;
    rt_device_t serial         = RT_NULL;
    uint32_t i                 = 0;

    serial = rt_device_find("uart3");
    if (serial == RT_NULL)
    {
        log_w("Can't find uart3, thread sleep.");
    }

    while (1)
    {
        rt_sem_take(hmi_sem, RT_WAITING_FOREVER);
        rt_device_read(serial, 0, &tmp, 1); /* 读取一个字节 */
        /* 接受开始符 */
        if (tmp == 0xFE)
        {
            if (rt_sem_take(hmi_sem, RT_TICK_PER_SECOND) == RT_ETIMEOUT)
            {
                log_w("Rec 0xFF timeout.");
                rt_memset(rx_buff, 0, i);
                i = 0;
                continue;
            }
            rt_device_read(serial, 0, rx_buff + i, 1);
            /* 接收到终止符 */
            if (rx_buff[i] == 0xFF)
            {
                rx_buff[i] = 0;
                /* 运行指令 */
                if (hmi_cmd_run(rx_buff) == RT_NULL)
                {
                    log_w("Error command.");
                }
                rt_memset(rx_buff, 0, i);
                i = 0;
                continue;
            }
            i++;
        }
    }
}

/**
 * @brief  串口3接受的钩子函数
 * @param  None
 * @return None
 */
static rt_err_t uart3_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(hmi_sem);

    return RT_EOK;
}

static int hmi_create(void)
{
    rt_thread_t tid    = RT_NULL;
    rt_device_t serial = RT_NULL;

    /* 打开串口3 */
    serial = rt_device_find("uart3");
    if (serial != RT_NULL)
    {
        /* 以读写及中断接收方式打开串口设备 */
        rt_device_open(serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
        //// rt_device_write(serial, 0, str, rt_strlen(str));  // test
        /* 初始化信号量通信 */
        hmi_sem = rt_sem_create("shmi", 0, RT_IPC_FLAG_PRIO);
        /* 设置钩子函数 */
        rt_device_set_rx_indicate(serial, uart3_input);
    }
    else
        log_e("uart3 open fail");

    /* 创建线程 */
    tid = rt_thread_create("thmi", hmi_thread, RT_NULL, HMI_THREAD_STACK_SIZE, HMI_THREAD_PRIORITY,
                           20);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    else
        log_e("hmi create fail");

    return 0;
}
INIT_APP_EXPORT(hmi_create);
