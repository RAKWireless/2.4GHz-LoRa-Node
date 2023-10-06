#ifndef _AT_CMD_H
#define _AT_CMD_H
#include <stdint.h>

#define STACK_ID 0
#define MAX_CMD_LEN 40
#define MAX_PARAM_LEN 256
#define MAX_ARGV_SIZE 10

#define VERSION "v1.0.3"

typedef struct
{
    char cmd[MAX_CMD_LEN + 1];
    char params[MAX_PARAM_LEN + 1];
    int  argc;
    char* argv[MAX_ARGV_SIZE];
} AT_Command;

extern uint32_t g_confirm_status;

#endif