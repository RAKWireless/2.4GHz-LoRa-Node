#include "at_cmd.h"
#include <stdio.h>
#include <string.h>
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


#define STACK_ID 0
#define MAX_CMD_LEN 40   
#define MAX_PARAM_LEN 256 


static uint32_t frequency_hz=2402000000,tx_power_dbm=10,sf=1,bw=3,cr=0,preamble_size=14;

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


typedef struct
{
    char cmd[MAX_CMD_LEN + 1];     
    char params[MAX_PARAM_LEN + 1]; 
} AT_Command;


void handle_version(const AT_Command *cmd)
{
    am_util_stdio_printf("Version: 1.0.1\n");
}

void handle_reset(const AT_Command *cmd)
{
    am_util_stdio_printf("Resetting...\n");
    NVIC_SystemReset();
    
}

void handle_deveui(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0)
    {
        
        int i;
        for (i = 0; i < 8; i++)
        {
            am_util_stdio_printf("%02X", lora_params.dev_eui[i]);
        }
        am_util_stdio_printf("\n");
    }
    else if (strlen(cmd->params) == 16)
    {
        
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
				save_lora_params();
    }
    else
    {
        am_util_stdio_printf("Invalid parameter\n");
    }

    
}

void handle_joineui(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0)
    {
        
        int i;
        for (i = 0; i < 8; i++)
        {
            am_util_stdio_printf("%02X", lora_params.join_eui[i]);
        }
        am_util_stdio_printf("\n");
    }
    else if (strlen(cmd->params) == 16)
    {
        
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
				save_lora_params();
    }
    else
    {
        am_util_stdio_printf("Invalid parameter\n");
    }

   
}

void handle_appkey(const AT_Command *cmd)
{
    if (strcmp(cmd->params, "?") == 0)
    {
        
        int i;
        for (i = 0; i < 16; i++)
        {
            am_util_stdio_printf("%02X", lora_params.app_key[i]);
        }
        am_util_stdio_printf("\n");
    }
    else if (strlen(cmd->params) == 32)
    {
        
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
				save_lora_params();
    }
    else
    {
        am_util_stdio_printf("Invalid parameter\n");
    }

   
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
	  if( modem_get_test_mode_status( ) == true )
    {
        am_util_stdio_printf( "In test mode, please reset to exit RF test mode.\n");
        return ;
    }
	
		// pre join
		uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {0};
		memset(custom_datarate,lora_params.dr,SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH);
		smtc_modem_adr_set_profile( STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, custom_datarate );
		
		uint8_t rc = smtc_modem_set_class( STACK_ID, lora_params.class );
		if( rc != SMTC_MODEM_RC_OK )
		{
				SMTC_HAL_TRACE_WARNING( "smtc_modem_set_class failed: rc=(%d)\n", rc );
		}
//    smtc_modem_set_deveui(STACK_ID, lora_params.dev_eui);
//    smtc_modem_set_joineui(STACK_ID, lora_params.join_eui);
//    smtc_modem_set_nwkkey(STACK_ID, lora_params.app_key);

    smtc_modem_join_network(STACK_ID);
}



//    SMTC_MODEM_CLASS_A = 0x00,  //!< Modem class A
//    SMTC_MODEM_CLASS_B = 0x01,  //!< Modem class B
//    SMTC_MODEM_CLASS_C = 0x02,  //!< Modem class C
void handle_class(const AT_Command *cmd)
{
		char* class_info[10] = {"A","B","C"};
	  if (strcmp(cmd->params, "?") == 0)
		{ 
        am_util_stdio_printf("%d\n",lora_params.class);
    }
    else if (strlen(cmd->params) == 1)
    {
			
				lora_params.class = atoi(cmd->params);
				if (lora_params.class > 2)
			  {
          am_util_stdio_printf("Invalid parameter\n");
					return;
        }
			  smtc_modem_set_class( STACK_ID, lora_params.class );
				save_lora_params();
        am_util_stdio_printf("OK\n");
    }
    else
    {
        am_util_stdio_printf("Invalid parameter\n");
    }
}


void handle_nwm(const AT_Command *cmd)
{
		smtc_modem_test_start();
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
    am_util_stdio_printf("frequency_hz %u\n",frequency_hz);
		am_util_stdio_printf("tx_power_dbm %d\n",tx_power_dbm);
		am_util_stdio_printf("sf %d\n",sf);
		am_util_stdio_printf("bw %d\n",bw);
		am_util_stdio_printf("cr %d\n",cr);
		am_util_stdio_printf("preamble_size %d\n",preamble_size);
		am_util_stdio_printf("OK\n");
		return;
  }
  if (sscanf(cmd->params, "%d:%d:%d:%d:%d:%d",&frequency_hz,&tx_power_dbm,&sf,&bw,&cr,&preamble_size) != 6)
	{
		am_util_stdio_printf("Invalid parameter\n");
    return;
	}
	am_util_stdio_printf("OK\n");
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
     am_util_stdio_printf("Invalid parameter\n");
  }
  
    char data[MAX_PARAM_LEN + 1];
		if (sscanf(cmd->params,"%s",data) != 1)
    {
        am_util_stdio_printf("Invalid parameter\n");
        return;
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
    am_util_stdio_printf("Sending data : ");
    for (size_t i = 0; i < count; i++)
    {
        am_util_stdio_printf("%02x ", hex_data[i]);
    }
    am_util_stdio_printf("\n");
		
		if( modem_get_test_mode_status( ) == false )
    {
			smtc_modem_test_start();
    }
    smtc_modem_test_tx( hex_data,count,frequency_hz,
                        tx_power_dbm,  sf,  bw,
                        cr,  preamble_size, false );
		
		am_util_stdio_printf("OK\n");
}



void handle_trx(const AT_Command *cmd)
{	  
	
	  if( modem_get_test_mode_status( ) == false )
    {
			smtc_modem_test_start();
    }
	  smtc_modem_test_rx_continuous( frequency_hz, sf, bw, cr  );
		am_util_stdio_printf("OK\n");
}

void handle_trxnop(const AT_Command *cmd)
{	  
	smtc_modem_test_nop();
}


void handle_dr(const AT_Command *cmd)
{	  
	if (strcmp(cmd->params, "?") == 0)
	{ 
    am_util_stdio_printf("%d\n",lora_params.dr);
		am_util_stdio_printf("OK\n");
		return;
  }
  else if (strlen(cmd->params) == 1)
  {
				lora_params.dr = atoi(cmd->params);
				if (lora_params.dr > 5)
			  {
          am_util_stdio_printf("Invalid parameter\n");
					return;
        }
				uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {0};
				memset(custom_datarate,lora_params.dr,SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH);
	      smtc_modem_adr_set_profile( STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, custom_datarate );
				save_lora_params();
        am_util_stdio_printf("OK\n");
  }
  else
  {
        am_util_stdio_printf("Invalid parameter\n");
  }
}


typedef void (*AT_Handler)(const AT_Command *);


typedef struct
{
    const char *cmd;    
    AT_Handler handler; 
    const char *help;   
} AT_HandlerTable;





AT_HandlerTable handler_table[] = {
    {"AT+VERSION", handle_version, "Get firmware version"},
    {"AT+RESET", handle_reset, "Reset device"},
    {"AT+DEVEUI", handle_deveui, "Set/Get device EUI"},
    {"AT+JOINEUI", handle_joineui, "Set/Get join EUI"},
    {"AT+APPKEY", handle_appkey, "Set/Get application key"},
    {"AT+JOIN", handle_join, "Join network"},
    {"AT+SEND", handle_send, "Send data to server"},
	  {"AT+CLASS", handle_class, "Set/Get class (0-CLASSA 2-CLASSC)"},
		{"AT+DR", handle_dr, "Set/Get datarate (0-5)"},
		{"AT+TCONF",handle_p2p,"Set/Get RF test config\nExample :\nAT+TCONF=2403000000:13:1:3:0:10 \nfrequency_hz 2403000000\ntx_power_dbm 13\nsf 1-8 SF5-SF12\nbw 3-BW200 4-BW400 5-BW-800 6-BW1600\n"
		"cr 0-CR4/5 1-CR4/6 2-CR4/7 3-CR4/8\n"
		"preamble_size 10\n"},
		{"AT+TTX",handle_psend,"RF test tx,Example A+TTX=1122BBCC"},
		{"AT+TRX",handle_trx,"RF test rx continuously receive mode"},
		{"AT+TRXNOP",handle_trxnop,"RF test terminate an ongoing continuous rx mode"},
};

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


void process_AT_Command(const char *input)
{
    AT_Command cmd = parse_AT_Command(input);
    int num_handlers = sizeof(handler_table) / sizeof(handler_table[0]);
    for (int i = 0; i < num_handlers; i++)
    {
        if (strcasecmp(cmd.cmd, handler_table[i].cmd) == 0)
        {
            handler_table[i].handler(&cmd);
            return;
        }
    }
    am_util_stdio_printf("ERROR: Unknown command\n"); 
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
    static char input[MAX_CMD_LEN + MAX_PARAM_LEN + 3]; 
    static int i = 0;

    if (c == '\b')
    {
        
        if (i > 0)
        {
            i--;
            am_util_stdio_printf(" \b"); 
        }
        return;
    }

    if (i >= MAX_CMD_LEN + MAX_PARAM_LEN + 2)
    {
       
        i = 0;
        am_util_stdio_printf("ERROR: Input buffer overflow\n");
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
