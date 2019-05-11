/**
 * 指令解析库
 *
 * 使用方法:
 * 1.字符指令格式必须为 <cmd> <argv> 或 <cmd>=<argv>
 *   argv可缺省
 * 2.将指令加入到_cmd_list数组中
 *     格式 {
 *          .cmd = <指令>,
 *          .argv = NULL,
 *          .event_handle = <事件函数>,
 *         }
 * 3.请开启c99支持
 */
#ifndef __HMI_CMD_H
#define __HMI_CMD_H

#ifndef NULL
#define NULL (0)
#endif
#define CMD_HEAD_MAX_LEN (10)

typedef struct hmi_cmd
{
    char* cmd;
    char* argv;
    void (*event_handle)(char* argv);
} * hmi_cmd_t, hmi_cmd;

#ifdef __cplusplus
extern "C"
{
#endif

    hmi_cmd_t hmi_cmd_analysis(char* cmd);
    hmi_cmd_t hmi_cmd_run(char* cmd);

#ifdef __cplusplus
}
#endif

#endif  // __HMI_CMD_H
