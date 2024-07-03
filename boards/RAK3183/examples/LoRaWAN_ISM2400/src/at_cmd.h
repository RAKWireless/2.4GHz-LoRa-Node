#ifndef _AT_CMD_H
#define _AT_CMD_H
#include <stdint.h>

#define STACK_ID 0
#define MAX_CMD_LEN 64
#define MAX_PARAM_LEN 256
#define MAX_ARGV_SIZE 64

#define VERSION "v1.0.4.1"

/* Error code definition */
typedef enum AT_ERRNO_E_{
        AT_OK = 0,
        AT_ERROR,
        AT_PARAM_ERROR,
        AT_BUSY_ERROR,
        AT_TEST_PARAM_OVERFLOW,
        AT_NO_CLASSB_ENABLE,
        AT_NO_NETWORK_JOINED,
        AT_RX_ERROR,
        AT_MODE_NO_SUPPORT,
        AT_COMMAND_NOT_FOUND,
        AT_UNSUPPORTED_BAND,
}AT_ERRNO_E;


typedef struct
{
    char cmd[MAX_CMD_LEN + 1];
    char params[MAX_PARAM_LEN + 1];
    int  argc;
    char* argv[MAX_ARGV_SIZE];
} AT_Command;


//Add a standard error code return value
typedef int (*AT_Handler)(const AT_Command *);

typedef struct
{
    const char *cmd;
    AT_Handler handler;
    const char *help;

} AT_HandlerTable;


extern uint32_t g_confirm_status;

#endif
