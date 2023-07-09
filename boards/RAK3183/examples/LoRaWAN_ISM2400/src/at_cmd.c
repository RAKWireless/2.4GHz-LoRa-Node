#include "at_cmd.h"
#include <stdio.h>
#include <string.h>
#include "tx_rx_test.h"
#include "am_mcu_apollo.h"
#include "am_util.h"

#include "smtc_modem_api.h"
#include "smtc_modem_utilities.h"

#include "smtc_modem_hal.h"
#include "smtc_hal_dbg_trace.h"

#include "ralf_sx128x.h"

#include "smtc_hal_mcu.h"
#include "smtc_hal_gpio.h"

#include "hal/am_hal_gpio.h"
#include "smtc_modem_test_api.h"
#include "smtc_modem_utilities.h"

#define STACK_ID 0
#define MAX_CMD_LEN 40   // 最大AT指令长度
#define MAX_PARAM_LEN 40 // 最大AT指令参数长度

static bool is_joined(void)
{
    uint32_t status = 0;
    smtc_modem_get_status(STACK_ID, &status);
    if ((status & SMTC_MODEM_STATUS_JOINED) == SMTC_MODEM_STATUS_JOINED)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int hex_string_to_bytes(const char *str, uint8_t *bytes, size_t len)
{
    char hex_char[3] = {0};
    hex_char[2] = '\0';
    for (size_t i = 0; i < len; i++)
    {
        hex_char[0] = str[i * 2];
        hex_char[1] = str[i * 2 + 1];
        char *endptr;
        long value = strtol(hex_char, &endptr, 16);
        if (endptr != hex_char + 2 || value < 0 || value > 255)
        {
            return -1;
        }
        bytes[i] = (uint8_t)value;
    }
    return 0;
}

// AT指令结构体，包括指令和参数
typedef struct
{
    char cmd[MAX_CMD_LEN + 1];      // 存储AT指令的字符数组
    char params[MAX_PARAM_LEN + 1]; // 存储AT指令的参数
} AT_Command;

// AT指令处理函数，根据需要实现不同的处理函数
void handle_version(const AT_Command *cmd)
{
    am_util_stdio_printf("Version: 1.0.0\n");
}

void handle_reset(const AT_Command *cmd)
{
    am_util_stdio_printf("Resetting...\n");
    NVIC_SystemReset();
    // 执行重启操作
}

void handle_deveui(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0)
    {
        // 输出当前设备的 DevEUI
        int i;
        for (i = 0; i < 8; i++)
        {
            am_util_stdio_printf("%02X", lora_params.dev_eui[i]);
        }
        am_util_stdio_printf("\n");
    }
    else if (strlen(cmd->params) == 16)
    {
        // 将参数解析为十六进制数，并保存到 user_dev_eui 数组中
        uint8_t bytes[8] = {0};
        if (hex_string_to_bytes(cmd->params, bytes, sizeof(bytes)) != 0)
        {
            am_util_stdio_printf("Invalid parameter\n");
            return;
        }
        memcpy(lora_params.dev_eui, bytes, sizeof(lora_params.dev_eui));
        am_util_stdio_printf("DevEUI set to ");
        for (int i = 0; i < 8; i++)
        {
            am_util_stdio_printf("%02X", lora_params.dev_eui[i]);
        }
        am_util_stdio_printf("\n");
    }
    else
    {
        am_util_stdio_printf("Invalid parameter\n");
    }

    save_lora_params();
}

void handle_joineui(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0)
    {
        // 输出当前设备的 DevEUI
        int i;
        for (i = 0; i < 8; i++)
        {
            am_util_stdio_printf("%02X", lora_params.join_eui[i]);
        }
        am_util_stdio_printf("\n");
    }
    else if (strlen(cmd->params) == 16)
    {
        // 将参数解析为十六进制数，并保存到 user_dev_eui 数组中
        uint8_t bytes[8] = {0};
        if (hex_string_to_bytes(cmd->params, bytes, sizeof(bytes)) != 0)
        {
            am_util_stdio_printf("Invalid parameter\n");
            return;
        }
        memcpy(lora_params.join_eui, bytes, sizeof(lora_params.join_eui));
        am_util_stdio_printf("DevEUI set to ");
        for (int i = 0; i < 8; i++)
        {
            am_util_stdio_printf("%02X", lora_params.join_eui[i]);
        }
        am_util_stdio_printf("\n");
    }
    else
    {
        am_util_stdio_printf("Invalid parameter\n");
    }

    save_lora_params();
}

void handle_appkey(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0)
    {
        // 输出当前设备的 DevEUI
        int i;
        for (i = 0; i < 16; i++)
        {
            am_util_stdio_printf("%02X", lora_params.app_key[i]);
        }
        am_util_stdio_printf("\n");
    }
    else if (strlen(cmd->params) == 32)
    {
        // 将参数解析为十六进制数，并保存到 user_dev_eui 数组中
        uint8_t bytes[16] = {0};
        if (hex_string_to_bytes(cmd->params, bytes, sizeof(bytes)) != 0)
        {
            am_util_stdio_printf("Invalid parameter\n");
            return;
        }
        memcpy(lora_params.app_key, bytes, sizeof(lora_params.app_key));
        am_util_stdio_printf("DevEUI set to ");
        for (int i = 0; i < 16; i++)
        {
            am_util_stdio_printf("%02X", lora_params.app_key[i]);
        }
        am_util_stdio_printf("\n");
    }
    else
    {
        am_util_stdio_printf("Invalid parameter\n");
    }

    save_lora_params();
}

void handle_send(const AT_Command *cmd)
{

    int port, confirm;
    char data[MAX_PARAM_LEN + 1];
    if (sscanf(cmd->params, "%d:%d:%s", &port, &confirm, data) != 3)
    {
        am_util_stdio_printf("Invalid parameter\n");
        return;
    }

    if (confirm > 1)
    {
        am_util_stdio_printf("Invalid hex value\n");
    }

    size_t len = strlen(data);
    if (len % 2 != 0)
    {
        am_util_stdio_printf("Invalid hex value\n");
        return;
    }
    unsigned char hex_data[256];
    size_t count = 0;
    for (size_t i = 0; i < len; i += 2)
    {
        unsigned int hex_value;
        if (sscanf(data + i, "%02x", &hex_value) != 1)
        {
            am_util_stdio_printf("Failed to parse hex value\n");
            return;
        }
        hex_data[count++] = (unsigned char)hex_value;
    }
    am_util_stdio_printf("Sending data on port %d confirm %d: ", port, confirm);
    for (size_t i = 0; i < count; i++)
    {
        am_util_stdio_printf("%02x ", hex_data[i]);
    }
    am_util_stdio_printf("\n");
    // 将 hex_data 发送出去的操作

    if (is_joined() == true)
    {
        uint8_t temperature = (uint8_t)smtc_modem_hal_get_temperature();
        smtc_modem_request_uplink(STACK_ID, port, confirm, hex_data, count);
    }
    else
    {
        am_util_stdio_printf("Device not joined to the network\n");
    }
}

void handle_join(const AT_Command *cmd)
{

    smtc_modem_set_deveui(STACK_ID, lora_params.dev_eui);
    smtc_modem_set_joineui(STACK_ID, lora_params.join_eui);
    smtc_modem_set_nwkkey(STACK_ID, lora_params.app_key);

    smtc_modem_join_network(STACK_ID);
}

// AT指令处理函数指针类型
typedef void (*AT_Handler)(const AT_Command *);

// AT指令处理函数表，包括指令和对应的处理函数
typedef struct
{
    const char *cmd;    // AT指令
    AT_Handler handler; // 对应的处理函数
    const char *help;   // 新增帮助信息字段
} AT_HandlerTable;

// AT指令处理函数表，可根据需要添加其他处理函数
AT_HandlerTable handler_table[] = {
    {"AT+VERSION", handle_version, "Get firmware version"},
    {"AT+RESET", handle_reset, "Reset device"},
    {"AT+DEVEUI", handle_deveui, "Set/Get device EUI"},
    {"AT+JOINEUI", handle_joineui, "Set/Get join EUI"},
    {"AT+APPKEY", handle_appkey, "Set/Get application key"},
    {"AT+JOIN", handle_join, "Join network"},
    {"AT+SEND", handle_send, "Send data to server"}};

AT_Command parse_AT_Command(const char *input)
{
    AT_Command cmd;
    const char *eq_pos = strchr(input, '=');
    if (eq_pos != NULL)
    {
        size_t cmd_len = eq_pos - input;
        size_t params_len = strlen(eq_pos + 1);
        memcpy(cmd.cmd, input, cmd_len);
        memcpy(cmd.params, eq_pos + 1, params_len);
        cmd.cmd[cmd_len] = '\0';
        cmd.params[params_len] = '\0';
    }
    else
    {
        strcpy(cmd.cmd, input);
        cmd.params[0] = '\0';
    }
    return cmd;
}

// 查找匹配的处理函数并执行
void process_AT_Command(const char *input)
{
    AT_Command cmd = parse_AT_Command(input);
    int num_handlers = sizeof(handler_table) / sizeof(handler_table[0]);
    for (int i = 0; i < num_handlers; i++)
    {
        if (strcasecmp(cmd.cmd, handler_table[i].cmd) == 0)
        {
            handler_table[i].handler(&cmd); // 执行匹配的处理函数
            return;
        }
    }
    am_util_stdio_printf("ERROR: Unknown command\n"); // 没有找到匹配的处理函数
}

void get_all_commands()
{
    am_util_stdio_printf("Available AT commands:\n");
    int num_handlers = sizeof(handler_table) / sizeof(handler_table[0]);
    for (int i = 0; i < num_handlers; i++)
    {
        am_util_stdio_printf("%s - %s\n", handler_table[i].cmd, handler_table[i].help);
    }
}

void process_serial_input(char c)
{
    static char input[MAX_CMD_LEN + MAX_PARAM_LEN + 3]; // 存储输入的AT指令和参数（包括换行符）
    static int i = 0;

    if (c == '\b')
    {
        // 如果读到了退格符，就删除上一个字符
        if (i > 0)
        {
            i--;
            am_util_stdio_printf(" \b"); // 输出退格符和空格符以删除上一个字符
        }
        return;
    }

    if (i >= MAX_CMD_LEN + MAX_PARAM_LEN + 2)
    {
        // 输入缓冲区已满，忽略后续字符
        i = 0;
        am_util_stdio_printf("ERROR: Input buffer overflow\n");
        return;
    }

    // 如果读到了换行符或回车符，就认为是一个AT指令
    if (c == '\n' || c == '\r')
    {
        input[i] = '\0'; // 加上字符串结束符
        if (strcasecmp(input, "AT?") == 0 || strcasecmp(input, "AT+HELP") == 0)
        {
            get_all_commands();
        }
        else
        {
            process_AT_Command(input);
        }
        i = 0; // 重置计数器
    }
    else
    { // 如果还没达到最大长度，就将字符存入输入缓冲区
        input[i] = c;
        i++;
    }
}
