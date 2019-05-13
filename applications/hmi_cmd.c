#include "hmi_cmd.h"
#include <stdint.h>
#include <string.h>

/* User Cmd List */
extern void adf4351_hmi_handle(char *argv);

static hmi_cmd _cmd_list[] = {
    {
        .cmd          = "freq",
        .argv         = NULL,
        .event_handle = adf4351_hmi_handle,
    },
};

#define hmi_assert(expr) ((expr) ? 0U : 1U)

/**
 * @brief  解析的字符串为指令
 * @param  cmd 字符串
 * @return 解析正确返回指令指针 否则返回空
 */
hmi_cmd_t hmi_cmd_analysis(char *cmd)
{
    hmi_cmd_t res = NULL;
    uint32_t i = 0, len = 0;
    char cmd_head[CMD_HEAD_MAX_LEN + 1] = {0};
    char *tmp_argv                      = NULL;

    if (hmi_assert(cmd != NULL))
    {
        return res;
    }

    /* 获取命令头 */
    while ((*(cmd + i) != ' ' && *(cmd + i) != '=') && (i < CMD_HEAD_MAX_LEN))
    {
        *(cmd_head + i) = *(cmd + i);
        i++;
    }
    cmd_head[i++] = '\0';
    len = strlen(cmd_head);
    if (hmi_assert(len <= CMD_HEAD_MAX_LEN))
    {
        return res;
    }

    tmp_argv      = cmd + i; /* 获取参数指针 */

    /* 遍历查找 */
    i = 0;
    for (i = 0; i < sizeof(_cmd_list); i++)
    {
        if (strcmp(cmd_head, _cmd_list[i].cmd))
        {
            continue;
        }
        /* 找到指令,配置参数 */
        res       = &_cmd_list[i];
        res->argv = tmp_argv;
        break;
    }

    return res;
}

/**
 * @brief  解析的字符串为指令并运行
 * @param  cmd 字符串
 * @return 解析正确返回指令指针 否则返回空
 */
hmi_cmd_t hmi_cmd_run(char *cmd)
{
    hmi_cmd_t res = 0;

    res = hmi_cmd_analysis(cmd);

    if (hmi_assert(res != NULL))
    {
        return res;
    }
    res->event_handle(res->argv);

    return res;
}
