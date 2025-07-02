#include "at_cmd.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lorawan_app.h"
#include "am_mcu_apollo.h"
#include "am_util.h"
#include "stdlib.h"

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

#include "smtc_hal_mcu.h"
#include "lr1mac_defs.h"

volatile int32_t board_delay_ms = 0;

static uint32_t frequency_hz = 2402000000, tx_power_dbm = 10, sf = 1, bw = 3, cr = 0, preamble_size = 14;
uint32_t g_confirm_status = 0;

enum mode
{
    P2P_MODE,
    LORAWAN_MODE
};

const char *atcmd_err_tbl[] =
    {
        "OK",
        "AT_ERROR",
        "AT_PARAM_ERROR",
        "AT_BUSY_ERROR",
        "AT_TEST_PARAM_OVERFLOW",
        "AT_NO_CLASSB_ENABLE",
        "AT_NO_NETWORK_JOINED",
        "AT_RX_ERROR",
        "AT_MODE_NO_SUPPORT",
        "AT_COMMAND_NOT_FOUND",
        "AT_UNSUPPORTED_BAND",
};

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

int handle_join_status(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        uint8_t status = is_joined();
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, status);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else
    {
        return AT_ERROR;
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

int handle_version(const AT_Command *cmd)
{
    am_util_stdio_printf("%s=%s\r\n", cmd->cmd, VERSION);
    return AT_OK;
}

void handle_reset(const AT_Command *cmd)
{
    // am_util_stdio_printf("Resetting...\r\n");
    NVIC_SystemReset();
}

int handle_hwmodel(const AT_Command *cmd)
{
    uint8_t temp_c = cmd->cmd[0];
    // am_util_stdio_printf("cmd: %s\r\n", cmd->cmd);    
    if(isupper(temp_c)){
        am_util_stdio_printf("RAK3183\r\n");
        return AT_OK;
    }else{
        am_util_stdio_printf("rak3183\r\n");
        return AT_OK;
    }
}

int handle_hwid(const AT_Command *cmd)
{
    uint8_t temp_c = cmd->cmd[0];    
    if(isupper(temp_c)){
        am_util_stdio_printf("APOLLO3 BLUE\r\n");
        return AT_OK;
    }
    else
    {
        am_util_stdio_printf("apollo3 blue\r\n");
        return AT_OK;
    }
}

int handle_deveui(const AT_Command *cmd)
{
    // Query instruction
    if (strcmp(cmd->argv[0], "?") == 0 && cmd->argc == 1)
    {
        int i;

        am_util_stdio_printf("%s=",cmd->cmd);
        for (i = 0; i < 8; i++)
        {
            am_util_stdio_printf("%02X", lora_params.dev_eui[i]);
        }
        am_util_stdio_printf("\r\n");
        return AT_OK;
        
    }
    else if (strlen(cmd->argv[0]) == 16 && cmd->argc == 1) // Set instruction
    {
        
        uint8_t deveui_temp[8] = {0};
        smtc_modem_return_code_t ret;

        if (hex_string_to_bytes(cmd->params, deveui_temp, sizeof(deveui_temp)) != 0)
        {
            return AT_PARAM_ERROR;
        }

        ret = smtc_modem_set_deveui(STACK_ID, deveui_temp);
        if(SMTC_MODEM_RC_OK == ret)
        {
            memcpy(lora_params.dev_eui, deveui_temp, sizeof(lora_params.dev_eui));
            save_lora_params();
            return AT_OK;

        } else if (SMTC_MODEM_RC_BUSY == ret)
        {
            return AT_BUSY_ERROR;
        } else
        {
            return AT_ERROR;
        }
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_joineui(const AT_Command *cmd)
{
    // Query instruction
    if (strcmp(cmd->argv[0], "?") == 0 && cmd->argc == 1)
    {        
        am_util_stdio_printf("%s=", cmd->cmd);
        for (int i = 0; i < 8; i++)
        {            
            am_util_stdio_printf("%02X", lora_params.join_eui[i]);
        }
        am_util_stdio_printf("\r\n");
        return AT_OK;
    }
    else if (strlen(cmd->argv[0]) == 16 && cmd->argc == 1)    
    {
        // 判断参数是否是数字
        uint8_t joineui_temp[8] = {0};
        smtc_modem_return_code_t ret;
        if (hex_string_to_bytes(cmd->params, joineui_temp, sizeof(joineui_temp)) != 0)
        {            
            return AT_PARAM_ERROR;
        }
        // 判断参数是否可以配置成功
        ret = smtc_modem_set_joineui(STACK_ID, joineui_temp);
        if (SMTC_MODEM_RC_OK == ret)
        {
            memcpy(lora_params.join_eui, joineui_temp, sizeof(lora_params.join_eui));
            save_lora_params();    
            return AT_OK;    
        }        
        else if(SMTC_MODEM_RC_BUSY == ret)
        {
            return AT_BUSY_ERROR;
        }else
        {
            return AT_ERROR;
        }        
    }
    else    
    {
        return AT_PARAM_ERROR;
    }
    
}

int handle_appkey(const AT_Command *cmd)
{
    // 查询appkey
    if (strcmp(cmd->argv[0], "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=", cmd->cmd);
        for (int i = 0; i < 16; i++)
        {
            am_util_stdio_printf("%02X", lora_params.app_key[i]);
        }
        am_util_stdio_printf("\r\n");
        return AT_OK;        
    }
    else if (strlen(cmd->argv[0]) == 32 && cmd->argc == 1)
    {
        // 判断参数是否为十六进制数
        uint8_t appkey_temp[16] = {0};
        smtc_modem_return_code_t ret;
        if(hex_string_to_bytes(cmd->params, appkey_temp, sizeof(appkey_temp)) != 0)
        {
            return AT_PARAM_ERROR;
        }
        // 设置参数
        ret = smtc_modem_set_nwkkey(STACK_ID, appkey_temp);
        if (SMTC_MODEM_RC_OK == ret)
        {
            // 保存参数
            memcpy(lora_params.app_key, appkey_temp, sizeof(lora_params.app_key));
            save_lora_params();            
            return AT_OK;
        }
        else if (SMTC_MODEM_RC_BUSY ==ret)        
        {
            return AT_BUSY_ERROR;
        }else
        {
            return AT_ERROR;
        }
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_send(const AT_Command *cmd)
{
    int port , len;
    uint8_t data[MAX_PARAM_LEN + 1];

    if(cmd->argc != 2)
    {
        return AT_PARAM_ERROR;
    }

    char* endptr;
    port = strtol(cmd->argv[0], &endptr, 10);
    if(port == 0)
    {
        return AT_PARAM_ERROR;
    }

    if(strlen(cmd->argv[1])%2 != 0)
    {
        return AT_PARAM_ERROR;
    }

    len = strlen(cmd->argv[1])/2;
    if(hex_string_to_bytes(cmd->argv[1],data,len) == -1)
    {
        return AT_PARAM_ERROR;
    }


    // if (sscanf(cmd->params, "%d:%s", &port, data) != 2)
    // {
    //     am_util_stdio_printf("AT_PARAM_ERROR\r\n");
    //     return;
    // }

    if (lorawan_api_get_activation_mode() == ACTIVATION_MODE_OTAA)
    {
        if (is_joined() == true)
        {
            smtc_modem_request_uplink(STACK_ID, port, lora_params.confirm, data, len);
            return AT_OK;
        }
        else
        {
            return AT_NO_NETWORK_JOINED;
        }
    }
    else
    {
        smtc_modem_request_uplink(STACK_ID, port, lora_params.confirm, data, len);
        return AT_OK;
    }
}

void handle_tx_power(const AT_Command *cmd)
{
}

int handle_join(const AT_Command *cmd)
{
    if (modem_get_test_mode_status() == true)
    {
        return AT_MODE_NO_SUPPORT;
    }

    smtc_modem_return_code_t ret;
    ret = smtc_modem_join_network(STACK_ID);
    if(ret == SMTC_MODEM_RC_OK)
    {
        return AT_OK;
    }else if (ret == SMTC_MODEM_RC_BUSY)
    {
        return AT_BUSY_ERROR;
    }
    else
    {
        return AT_ERROR;
    }
    
}

//    SMTC_MODEM_CLASS_A = 0x00,  //!< Modem class A
//    SMTC_MODEM_CLASS_B = 0x01,  //!< Modem class B
//    SMTC_MODEM_CLASS_C = 0x02,  //!< Modem class C
int handle_class(const AT_Command *cmd)
{    
    // at+class=1, AT_ERROR, 与old一致
    // query class value
    if (strcmp(cmd->argv[0], "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.class);
        return AT_OK;
    }
    // set class value
    else if (strlen(cmd->params) == 1 && cmd->argc == 1)
    {
        uint8_t class_temp = atoi(cmd->params);
        // am_util_stdio_printf("class_temp=%d\r\n", class_temp);
        smtc_modem_return_code_t ret;
        if (class_temp > 2)
        {
            return AT_PARAM_ERROR;
        }
        ret = smtc_modem_set_class(STACK_ID, class_temp);
        if (SMTC_MODEM_RC_OK == ret)
        {
            lora_params.class = class_temp;
            save_lora_params();
            return AT_OK;
        }
        else if (SMTC_MODEM_RC_BUSY == ret)
        {
            return AT_BUSY_ERROR;
        }        
        else
        {    
            // am_util_stdio_printf("%d\r\n", ret);
            return AT_ERROR;
        }
    }    
}

int handle_nwm(const AT_Command *cmd)
{
    // query nwm value
    if (strcmp(cmd->argv[0], "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.nwm);
        return AT_OK;
    }
    // set nwm value
    else if (strlen(cmd->params) == 1 && cmd->argc == 1)
    {
        uint8_t nwm_temp = atoi(cmd->params);        
        if (nwm_temp > 1)
        {
            return AT_PARAM_ERROR;
        }
        else
        {
            lora_params.nwm = nwm_temp;
            save_lora_params();
            NVIC_SystemReset();
            return AT_OK;
        }        
    }
    else
    {
        return AT_ERROR;
    } 
}

int handle_freq(const AT_Command *cmd)
{
    if (lora_params.nwm == LORAWAN_MODE)
    {
        return AT_MODE_NO_SUPPORT;
    }

    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%u\r\n", cmd->cmd, lora_params.frequency_hz);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) == 10)
    {
        uint32_t frequency_hz = strtoul(cmd->params, NULL, 10);
        if (frequency_hz > 2500000000 || frequency_hz < 2400000000)
        {
            return AT_PARAM_ERROR;
        }
        lora_params.frequency_hz = frequency_hz;
        save_lora_params();
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_tx_power_dbm(const AT_Command *cmd)
{
    if (lora_params.nwm == LORAWAN_MODE)
    {
        return AT_MODE_NO_SUPPORT;
    }

    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.tx_power_dbm);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) < 3)
    {
        uint32_t power = atoi(cmd->params);
        if (power > 13)
        {
            return AT_PARAM_ERROR;
        }
        lora_params.tx_power_dbm = power;
        save_lora_params();
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_sf(const AT_Command *cmd)
{
    if (lora_params.nwm == LORAWAN_MODE)
    {
        return AT_MODE_NO_SUPPORT;
    }

    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.sf);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) < 3)
    {
        uint32_t sf = atoi(cmd->params);
        if (sf > 8 || sf < 1)
        {
            return AT_PARAM_ERROR;
        }
        lora_params.sf = sf;
        save_lora_params();
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_bw(const AT_Command *cmd)
{
    if (lora_params.nwm == LORAWAN_MODE)
    {
        return AT_MODE_NO_SUPPORT;
    }

    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.bw);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) == 1)
    {
        uint32_t bw = atoi(cmd->params);
        if (bw > 6 || bw < 3)
        {
            return AT_PARAM_ERROR;
        }
        lora_params.bw = bw;
        save_lora_params();
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_cr(const AT_Command *cmd)
{
    if (lora_params.nwm == LORAWAN_MODE)
    {
        return AT_MODE_NO_SUPPORT;
    }

    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.cr);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) == 1)
    {
        uint32_t cr = atoi(cmd->params);
        if (cr > 3)
        {
            return AT_PARAM_ERROR;
        }
        lora_params.cr = cr;
        save_lora_params();
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_preamble_size(const AT_Command *cmd)
{
    if (lora_params.nwm == LORAWAN_MODE)
    {
        return AT_MODE_NO_SUPPORT;
    }

    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.preamble_size);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) > 0)
    {
        uint32_t preamble = atoi(cmd->params);
        lora_params.preamble_size = preamble;
        save_lora_params();
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_p2p_precv(const AT_Command *cmd)
{
    if (lora_params.nwm == LORAWAN_MODE)
    {
        return AT_MODE_NO_SUPPORT;
    }

    if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }

    uint32_t time = atoi(cmd->params);

    if (time == 0 && cmd->params[0] != '0')
    {
        smtc_modem_test_nop();
    }
    else if (time == 65535 || time == 65534)
    {
        smtc_modem_test_rx_continuous(lora_params.frequency_hz, lora_params.sf, lora_params.bw, lora_params.cr);
    }
    else
    {
        /*to timer   smtc_modem_alarm_start_timer ERROR*/
        smtc_modem_test_rx_continuous(lora_params.frequency_hz, lora_params.sf, lora_params.bw, lora_params.cr);
    }
    return AT_OK;
}

void handle_p2p_send(const AT_Command *cmd)
{
    if (lora_params.nwm == LORAWAN_MODE)
    {
        am_util_stdio_printf("MODE_NOT_SUPPORT\r\n");
        return;
    }

    if (strcmp(cmd->params, "?") == 0)
    {
        am_util_stdio_printf("AT_PARAM_ERROR\r\n");
    }

    char data[MAX_PARAM_LEN + 1];
    if (sscanf(cmd->params, "%s", data) != 1)
    {
        am_util_stdio_printf("AT_PARAM_ERROR\r\n");
        return;
    }
    size_t len = strlen(data);
    if (len % 2 != 0)
    {
        am_util_stdio_printf("Invalid hex value\r\n");
        return;
    }
    unsigned char hex_data[256];
    size_t count = 0;
    for (size_t i = 0; i < len; i += 2)
    {
        unsigned int hex_value;
        if (sscanf(data + i, "%02x", &hex_value) != 1)
        {
            am_util_stdio_printf("Failed to parse hex value\r\n");
            return;
        }
        hex_data[count++] = (unsigned char)hex_value;
    }

    if (modem_get_test_mode_status() == false)
    {
        smtc_modem_test_start();
    }
    smtc_modem_test_tx(hex_data, count, lora_params.frequency_hz,
                       lora_params.tx_power_dbm, lora_params.sf, lora_params.bw,
                       lora_params.cr, lora_params.preamble_size, false);

    am_util_stdio_printf("OK\r\n");
}

/**
 * @brief Test mode TX single or continue
 * @remark Transmit a single packet or continuously transmit packets as fast as possible.
 *
 * @param [in] payload*        Payload that will be sent. If NULL a randomly generated payload_length msg will be sent
 * @param [in] payload_length  Length of the payload
 * @param [in] frequency_hz    Frequency in Hz
 * @param [in] tx_power_dbm    Power in dbm
 * @param [in] sf              Spreading factor following smtc_modem_test_sf_t definition
 * @param [in] bw              Bandwith following smtc_modem_test_bw_t definition
 * @param [in] cr              Coding rate following smtc_modem_test_cr_t definition
 * @param [in] preamble_size   Size of the preamble
 * @param [in] continuous_tx   false: single transmission / true: continuous transmission
 *
 * @return Modem return code as defined in @ref smtc_modem_return_code_tl
 */

void handle_p2p(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0)
    {
        am_util_stdio_printf("frequency_hz %u\r\n", frequency_hz);
        am_util_stdio_printf("tx_power_dbm %d\r\n", tx_power_dbm);
        am_util_stdio_printf("sf %d\r\n", sf);
        am_util_stdio_printf("bw %d\r\n", bw);
        am_util_stdio_printf("cr %d\r\n", cr);
        am_util_stdio_printf("preamble_size %d\r\n", preamble_size);
        am_util_stdio_printf("OK\r\n");
        return;
    }
    if (sscanf(cmd->params, "%d:%d:%d:%d:%d:%d", &frequency_hz, &tx_power_dbm, &sf, &bw, &cr, &preamble_size) != 6)
    {
        am_util_stdio_printf("AT_PARAM_ERROR\r\n");
        return;
    }
    am_util_stdio_printf("OK\r\n");
    /*
        SMTC_MODEM_TEST_FSK = 0,        //!< FSK
        SMTC_MODEM_TEST_LORA_SF5,       //!< SF5
        SMTC_MODEM_TEST_LORA_SF6,       //!< SF6
        SMTC_MODEM_TEST_LORA_SF7,       //!< SF7
        SMTC_MODEM_TEST_LORA_SF8,       //!< SF8
        SMTC_MODEM_TEST_LORA_SF9,       //!< SF9
        SMTC_MODEM_TEST_LORA_SF10,      //!< SF10
        SMTC_MODEM_TEST_LORA_SF11,      //!< SF11
        SMTC_MODEM_TEST_LORA_SF12,      //!< SF12
        SMTC_MODEM_TEST_LORA_SF_COUNT,  //!< Count
    */

    /*
        SMTC_MODEM_TEST_BW_125_KHZ = 0,   //!< BW125
        SMTC_MODEM_TEST_BW_250_KHZ,   //!< BW250
        SMTC_MODEM_TEST_BW_500_KHZ,   //!< BW500
        SMTC_MODEM_TEST_BW_200_KHZ,   //!< BW200
        SMTC_MODEM_TEST_BW_400_KHZ,   //!< BW400
        SMTC_MODEM_TEST_BW_800_KHZ,   //!< BW800
        SMTC_MODEM_TEST_BW_1600_KHZ,  //!< BW1600
        SMTC_MODEM_TEST_BW_COUNT,     //!< Count

        SMTC_MODEM_TEST_BW_12M = 15,
        SMTC_MODEM_TEST_BW_18M,
        SMTC_MODEM_TEST_BW_24M,
    */

    /*
        SMTC_MODEM_TEST_CR_4_5 = 0,  //!< CR 4/5
        SMTC_MODEM_TEST_CR_4_6,      //!< CR 4/6
        SMTC_MODEM_TEST_CR_4_7,      //!< CR 4/7
        SMTC_MODEM_TEST_CR_4_8,      //!< CR 4/8
        SMTC_MODEM_TEST_CR_LI_4_5,   //!< CR 4/5 long interleaved
        SMTC_MODEM_TEST_CR_LI_4_6,   //!< CR 4/6 long interleaved
        SMTC_MODEM_TEST_CR_LI_4_8,   //!< CR 4/8 long interleaved
        SMTC_MODEM_TEST_CR_COUNT,    //!< Count
    */
}

void handle_psend(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0)
    {
        am_util_stdio_printf("AT_PARAM_ERROR\r\n");
    }

    char data[MAX_PARAM_LEN + 1];
    if (sscanf(cmd->params, "%s", data) != 1)
    {
        am_util_stdio_printf("AT_PARAM_ERROR\r\n");
        return;
    }
    size_t len = strlen(data);
    if (len % 2 != 0)
    {
        am_util_stdio_printf("Invalid hex value\r\n");
        return;
    }
    unsigned char hex_data[256];
    size_t count = 0;
    for (size_t i = 0; i < len; i += 2)
    {
        unsigned int hex_value;
        if (sscanf(data + i, "%02x", &hex_value) != 1)
        {
            am_util_stdio_printf("Failed to parse hex value\r\n");
            return;
        }
        hex_data[count++] = (unsigned char)hex_value;
    }
    am_util_stdio_printf("Sending data : ");
    for (size_t i = 0; i < count; i++)
    {
        am_util_stdio_printf("%02x ", hex_data[i]);
    }
    am_util_stdio_printf("\r\n");

    if (modem_get_test_mode_status() == false)
    {
        smtc_modem_test_start();
    }
    smtc_modem_test_tx(hex_data, count, frequency_hz,
                       tx_power_dbm, sf, bw,
                       cr, preamble_size, false);

    am_util_stdio_printf("OK\r\n");
}

void handle_trx(const AT_Command *cmd)
{

    if (modem_get_test_mode_status() == false)
    {
        smtc_modem_test_start();
    }
    smtc_modem_test_rx_continuous(frequency_hz, sf, bw, cr);
    am_util_stdio_printf("OK\r\n");
}

void handle_trxnop(const AT_Command *cmd)
{
    smtc_modem_test_nop();
}

int handle_dr(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.dr);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) == 1)
    {
        uint8_t dr = atoi(cmd->params);
        if (dr > 5)
        {
            return AT_PARAM_ERROR;
        }
        lora_params.dr = dr;
        uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {0};
        memset(custom_datarate, lora_params.dr, SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH);
        smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, custom_datarate);
        save_lora_params();
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_sendinterval(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%u\r\n", cmd->cmd, lora_params.interval);        
        return AT_OK;
    }
    else if (cmd->params > 0 && cmd->argc == 1)
    {
        lora_params.interval = atoi(cmd->params);
        save_lora_params();
        return AT_OK;
    }
    else
    {
        return AT_ERROR;
    }
    if (lora_params.interval > 0)
        smtc_modem_alarm_start_timer(lora_params.interval);
}

int handle_compensation(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, board_delay_ms);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) > 0)
    {
        board_delay_ms = atoi(cmd->params);
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_confirm_status(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, g_confirm_status);
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else
    {
        return AT_ERROR;
    }
}

int handle_confirm(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd, lora_params.confirm);        
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }    
    else if (strlen(cmd->params) == 1)
    {
        uint8_t confirm_temp = atoi(cmd->params);
        if (confirm_temp > 1)
        {            
            return AT_PARAM_ERROR;
        }
        else
        {
            lora_params.confirm = confirm_temp;
            save_lora_params();
            return AT_OK;
        }        
    }
    else
    {
        return AT_ERROR;
    }
}

int handle_test(const AT_Command *cmd)
{
    am_util_stdio_printf("AT_Command\r\n");
    am_util_stdio_printf("cmd->cmd %s\r\n", cmd->cmd);
    am_util_stdio_printf("cmd->params %s\r\n", cmd->params);
    am_util_stdio_printf("cmd->argc %d\r\n", cmd->argc);

    for (int i = 0; i < cmd->argc; i++)
    {
        am_util_stdio_printf("cmd->argv[%d] : %s\r\n", i, cmd->argv[i]);
    }

    am_util_stdio_printf("OK\r\n");
    return AT_OK;
}

int handle_njm(const AT_Command *cmd)
{
    // query njm value
    if (strcmp(cmd->argv[0], "?") == 0 && cmd->argc ==1)
    {        
        am_util_stdio_printf("%s=", cmd->cmd);
        am_util_stdio_printf("%d\r\n", lora_params.join_mode);        
        return AT_OK;
    }    
    else if (strlen(cmd->params) < 2 && cmd->argc == 1)
    {
        uint8_t mode = 0;
        mode = atoi(cmd->params);
        // NJM大于1时，参数非法
        if (mode > 1)
        {
            return AT_PARAM_ERROR;
        }        
        // 和RUI保持一致  0:ABP 1:OTAA
        if (!mode)
            lorawan_api_set_activation_mode(!mode);

        lora_params.join_mode = mode;
        save_lora_params();  
        return AT_OK;      
    }
    else
    {
        return AT_PARAM_ERROR;
    }    
}

int handle_devaddr(const AT_Command *cmd)
{
    // query devaddr value
    if (strcmp(cmd->argv[0], "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=", cmd->cmd);
        am_util_stdio_printf("%08X\r\n", lora_params.devaddr);        
        return AT_OK;
    }
    else if (strlen(cmd->params) == 8 && cmd->argc == 1)
    {        
        uint32_t devaddr_temp = 0;
        // check value
        if (sscanf(cmd->params, "%8X", &devaddr_temp) != 1)
        {
            return AT_PARAM_ERROR;
        }
        else
        {
            lorawan_api_devaddr_set(devaddr_temp);
            // memcpy(lora_params.devaddr, devaddr_temp, sizeof(lora_params.devaddr));  // Hard Fault stacked data: R0 = 0x1234567
            lora_params.devaddr = devaddr_temp;
            save_lora_params();
            return AT_OK;
        }                
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_rety(const AT_Command *cmd)
{
    uint8_t retry = 0;
    smtc_modem_return_code_t ret;
    
    if (strcmp(cmd->argv[0], "?") == 0 && cmd->argc == 1)
    {        
        am_util_stdio_printf("%s=%d\r\n", cmd->cmd,  lora_params.retry);
        return AT_OK;
    }
    else if (strlen(cmd->params) < 3) /* It is more appropriate to determine the number of parameters instead */
    {
        retry = atoi(cmd->params);

        if (retry > 15 || retry < 0)
        {            
            return AT_PARAM_ERROR;
        }

        lorawan_api_dr_strategy_set(USER_DR_DISTRIBUTION);

        ret = smtc_modem_set_nb_trans(0, retry);
        if (ret != 0)
        {
            // am_util_stdio_printf("AT_PARAM_ERROR\r\n");
            return AT_PARAM_ERROR;
        }

        lora_params.retry = retry;
        save_lora_params();
        // am_util_stdio_printf("OK\r\n");
        return AT_OK;
    }
    else
    {
        // am_util_stdio_printf("AT_PARAM_ERROR\r\n");
        return AT_PARAM_ERROR;
    }
    // return;
}

int handle_nwkskey(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=", cmd->cmd);
        for (int i = 0; i < 16; i++)
        {
            am_util_stdio_printf("%02X", lora_params.nwkskey[i]);
        }
        am_util_stdio_printf("\r\n");
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) == 32)
    {
        uint8_t bytes[16] = {0};
        if (hex_string_to_bytes(cmd->params, bytes, sizeof(bytes)) != 0)
        {
            return AT_PARAM_ERROR;
        }
        memcpy(lora_params.nwkskey, bytes, sizeof(lora_params.nwkskey));
        save_lora_params();
        smtc_modem_crypto_set_key(SMTC_SE_NWK_S_ENC_KEY, lora_params.nwkskey);
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

int handle_appskey(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0 && cmd->argc == 1)
    {
        am_util_stdio_printf("%s=", cmd->cmd);
        for (int i = 0; i < 16; i++)
        {
            am_util_stdio_printf("%02X", lora_params.appskey[i]);
        }
        am_util_stdio_printf("\r\n");
        return AT_OK;
    }
    else if (cmd->argc != 1)
    {
        return AT_PARAM_ERROR;
    }
    else if (strlen(cmd->params) == 32)
    {
        uint8_t bytes[16] = {0};
        if (hex_string_to_bytes(cmd->params, bytes, sizeof(bytes)) != 0)
        {
            return AT_PARAM_ERROR;
        }
        memcpy(lora_params.appskey, bytes, sizeof(lora_params.appskey));
        save_lora_params();
        smtc_modem_crypto_set_key(SMTC_SE_APP_S_KEY, lora_params.appskey);
        return AT_OK;
    }
    else
    {
        return AT_PARAM_ERROR;
    }
}

AT_HandlerTable handler_table[] = {
    // === System Commands ===
    {"ATZ",           handle_reset,        "System reset and restart device"},
    {"AT+VER",        handle_version,      "Get firmware version information"},
    {"AT+HWMODEL",    handle_hwmodel,      "Get hardware model string (RAK3183)"},
    {"AT+HWID",       handle_hwid,         "Get hardware ID string (APOLLO3 BLUE)"},

    // === LoRaWAN Device Configuration ===
    {"AT+DEVEUI",     handle_deveui,       "Set/Get LoRaWAN device EUI (8 bytes hex)"},
    {"AT+JOINEUI",    handle_joineui,      "Set/Get LoRaWAN join/application EUI (8 bytes hex)"},
    {"AT+APPKEY",     handle_appkey,       "Set/Get LoRaWAN application key (16 bytes hex)"},
    {"AT+DEVADDR",    handle_devaddr,      "Set/Get LoRaWAN device address (4 bytes hex)"},
    {"AT+NWKSKEY",    handle_nwkskey,      "Set/Get LoRaWAN network session key (16 bytes hex)"},
    {"AT+APPSKEY",    handle_appskey,      "Set/Get LoRaWAN application session key (16 bytes hex)"},
    {"AT+NJM",        handle_njm,          "Set/Get LoRaWAN join mode (0=ABP, 1=OTAA)"},

    // === LoRaWAN Network Operations ===
    {"AT+JOIN",       handle_join,         "Join LoRaWAN network"},
    {"AT+NJS",        handle_join_status,  "Get LoRaWAN join status (0=not joined, 1=joined)"},
    {"AT+SEND",       handle_send,         "Send LoRaWAN uplink data (port:hex_data)"},
    {"AT+CFM",        handle_confirm,      "Set/Get LoRaWAN confirm mode (0=unconfirmed, 1=confirmed)"},
    {"AT+CFS",        handle_confirm_status, "Get last transmission confirmation status (0=failed, 1=success)"},
    {"AT+CLASS",      handle_class,        "Set/Get LoRaWAN device class (0=Class A, 2=Class C)"},
    {"AT+DR",         handle_dr,           "Set/Get LoRaWAN data rate (0-5)"},
    {"AT+RETY",       handle_rety,         "Set/Get LoRaWAN retransmission count (0-15)"},

    // === Working Mode Configuration ===
    {"AT+NWM",        handle_nwm,          "Set/Get network working mode (0=P2P, 1=LoRaWAN)"},
    //{"AT+INTERVAL",   handle_sendinterval, "Set/Get automatic transmission interval (seconds)"},

    // === P2P Mode Configuration ===
    {"AT+PFREQ",      handle_freq,         "Set/Get P2P frequency (2400000000-2500000000 Hz)"},
    {"AT+PTP",        handle_tx_power_dbm, "Set/Get P2P TX power (0-13 dBm)"},
    {"AT+PSF",        handle_sf,           "Set/Get P2P spreading factor (1-8: SF5-SF12)"},
    {"AT+PBW",        handle_bw,           "Set/Get P2P bandwidth (3=200kHz, 4=400kHz, 5=800kHz, 6=1600kHz)"},
    {"AT+PCR",        handle_cr,           "Set/Get P2P coding rate (0=4/5, 1=4/6, 2=4/7, 3=4/8)"},
    {"AT+PPL",        handle_preamble_size, "Set/Get P2P preamble length"},
    {"AT+PSEND",      handle_p2p_send,     "Send P2P data packet (hex string)"},
    {"AT+PRECV",      handle_p2p_precv,    "Start/Stop P2P continuous receive (0=stop, others=start)"},

    // === RF Test Commands ===
    {"AT+TCONF",      handle_p2p,          "Set/Get RF test configuration (freq:power:sf:bw:cr:preamble)"},
    {"AT+TTX",        handle_psend,        "RF test: transmit packet (hex data)"},
    {"AT+TRX",        handle_trx,          "RF test: start continuous receive mode"},
    {"AT+TRXNOP",     handle_trxnop,       "RF test: stop continuous receive mode"},

    // === Debug and Test ===
    {"AT+TEST",       handle_test,         "Debug command for testing AT parser"}
};

AT_Command parse_AT_Command(const char *input)
{
    AT_Command cmd;
    const char *eq_pos = strchr(input, '=');
    if (eq_pos != NULL)
    {
        size_t cmd_len = eq_pos - input;
        /* Gets the length of the argument */
        size_t params_len = strlen(eq_pos + 1);
        memcpy(cmd.cmd, input, cmd_len);
        memcpy(cmd.params, eq_pos + 1, params_len);
        cmd.cmd[cmd_len] = '\0';
        cmd.params[params_len] = '\0';

        // 使用一个局部变量来分割参数，以保持cmd.params的完整性  需要静态变量 保证内存安全性
        static char params_copy[MAX_PARAM_LEN + 1];     
        strcpy(params_copy, cmd.params);

        char *token = strtok(params_copy, ":");
        int arg_index = 0;

        while (token != NULL && arg_index < MAX_ARGV_SIZE)
        {
            cmd.argv[arg_index++] = token;
            token = strtok(NULL, ":");
        }
        cmd.argc = arg_index;
    }
    else
    {
        /* Without the = sign, the whole string is copied */
        strcpy(cmd.cmd, input);
        cmd.params[0] = '\0';
        cmd.argc = 0;
    }

    return cmd;
}

int process_AT_Command(const char *input)
{
    int	ret = AT_ERROR;
    int i;
    AT_Command cmd = parse_AT_Command(input);
    //  start fix Hit Enter to return AT_ERROR
    if (strlen(cmd.cmd) == 0) // When the length of the AT command is 0, no processing is performed.
    {
        am_util_stdio_printf("\r\n");
        return ret;
    }
    //  end fix Hit Enter to return AT_ERROR

    int num_handlers = sizeof(handler_table) / sizeof(handler_table[0]);
    for ( i = 0; i < num_handlers; i++)
    {
        if (strcasecmp(cmd.cmd, handler_table[i].cmd) == 0)
        {
            am_util_stdio_printf("\r\n"); // Add \r\n before returning the result of the AT command.
            ret = handler_table[i].handler(&cmd);
            break;
        }
    }

    if(i == num_handlers)
    {
        ret = AT_COMMAND_NOT_FOUND;
    }

    if (ret < sizeof(atcmd_err_tbl)/sizeof(char *)) {
        am_util_stdio_printf("%s\r\n", atcmd_err_tbl[ret]);
    } else {
        am_util_stdio_printf("%s\r\n", atcmd_err_tbl[1]);
    }
		
		return ret;
}

void get_all_commands()
{
    am_util_stdio_printf("Available AT commands:\r\n");
    int num_handlers = sizeof(handler_table) / sizeof(handler_table[0]);
    for (int i = 0; i < num_handlers; i++)
    {
        am_util_stdio_printf("%s - %s\r\n", handler_table[i].cmd, handler_table[i].help);
    }
}

void process_serial_input(char c)
{
    static char input[MAX_CMD_LEN + MAX_PARAM_LEN + 3];
    static int i = 0;

    /* backspace */
    if (c == '\b')
    {
        if (i > 0)
        {
            i--;
            am_util_stdio_printf(" \b");
        }
        return;
    }

    if (i >= MAX_CMD_LEN + MAX_PARAM_LEN + 2) //+2 /r/n
    {
        i = 0;
        am_util_stdio_printf("ERROR: Input buffer overflow\r\n");
        return;
    }

    if (c == '\n' || c == '\r')
    {
        input[i] = '\0';
        if (strcasecmp(input, "AT?") == 0 || strcasecmp(input, "AT+HELP") == 0)
        {
            get_all_commands();
        }
        else
        {
            process_AT_Command(input);
        }
        i = 0;
    }
    else
    {
        input[i] = c;
        i++;
    }
}
