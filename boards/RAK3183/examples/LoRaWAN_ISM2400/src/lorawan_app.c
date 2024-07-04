#include "lorawan_app.h"
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
#include "smtc_hal_flash.h"
#include "lr1mac_defs.h"

#include "i2c.h"
#include "at_cmd.h"

#define ADDR_FLASH_AT_PARAM_CONTEXT (AM_HAL_FLASH_INSTANCE_SIZE + (4 * AM_HAL_FLASH_PAGE_SIZE))
#define APP_TX_DUTYCYCLE (7)

void data_lpp_uplink(void);
/**
 * @brief LoRaWAN User credentials
 */
#define USER_LORAWAN_DEVICE_EUI                        \
	{                                                  \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
	}
#define USER_LORAWAN_JOIN_EUI                          \
	{                                                  \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
	}
#define USER_LORAWAN_APP_KEY                               \
	{                                                      \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    \
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
	}

#define USER_LORAWAN_NWKSKEY                               \
	{                                                      \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    \
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
	}

#define USER_LORAWAN_APPSKEY                               \
	{                                                      \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    \
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
	}

#define MODEM_EXAMPLE_REGION SMTC_MODEM_REGION_WW2G4

/**
 * Stack id value (multistacks modem is not yet available)
 */
#define STACK_ID 0

/**
 * @brief Stack credentials  default parameters
 */

// SMTC_MODEM_CLASS_A = 0x00,  //!< Modem class A
// SMTC_MODEM_CLASS_B = 0x01,  //!< Modem class B
// SMTC_MODEM_CLASS_C = 0x02,  //!< Modem class C
static uint16_t calculate_crc_for_lorawan_params(const LoRaWAN_Params *params);

LoRaWAN_Params lora_params = {
	.dev_eui = USER_LORAWAN_DEVICE_EUI,
	.join_eui = USER_LORAWAN_JOIN_EUI,
	.app_key = USER_LORAWAN_APP_KEY,
	.devaddr = 0,
	.appskey = USER_LORAWAN_APPSKEY,
	.nwkskey = USER_LORAWAN_NWKSKEY,
	.class = 0,
	.dr = 0,
	.confirm = 1,
	.retry = 1,
	.join_mode = 1,

	.nwm = 1,
	.frequency_hz = 2402000000,
	.tx_power_dbm = 10,
	.sf = 1,
	.bw = 3,
	.cr = 0,
	.preamble_size = 14,
	.interval = 0};

uint8_t rx_payload_size;
uint8_t rx_payload[256];

#if defined(SX128X)
const ralf_t modem_radio = RALF_SX128X_INSTANTIATE(NULL);
#elif defined(SX126X)
const ralf_t modem_radio = RALF_SX126X_INSTANTIATE(NULL);
#elif defined(LR11XX)
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE(NULL);
#else
#error "Please select radio board.."
#endif

void save_lora_params(void)
{
	uint16_t crc ;
	crc = calculate_crc_for_lorawan_params(&lora_params);
	lora_params.crc = crc;

	//am_util_stdio_printf("save crc %04x\r\n",lora_params.crc);

	hal_flash_erase_page(ADDR_FLASH_AT_PARAM_CONTEXT, 1);
	hal_flash_write_buffer(ADDR_FLASH_AT_PARAM_CONTEXT, (uint8_t *)&lora_params, sizeof(lora_params));
	uint8_t stack_id = STACK_ID;
}

void load_lora_params(void)
{
	uint16_t crc ;
	LoRaWAN_Params lora_params_temp;
	hal_flash_read_buffer(ADDR_FLASH_AT_PARAM_CONTEXT, (uint8_t *)&lora_params_temp, sizeof(lora_params_temp));

	crc  = calculate_crc_for_lorawan_params(&lora_params_temp);
	
	if(lora_params_temp.crc != crc)
	{
		save_lora_params();
	} else
	{
		memcpy(&lora_params,&lora_params_temp,sizeof(lora_params));
	}

}

static void get_event(void)
{
	smtc_modem_event_t current_event;
	uint8_t event_pending_count;
	uint8_t stack_id = STACK_ID;

	// Continue to read modem event until all event has been processed
	do
	{
		// Read modem event
		smtc_modem_get_event(&current_event, &event_pending_count);

		switch (current_event.event_type)
		{
		case SMTC_MODEM_EVENT_RESET:
			SMTC_HAL_TRACE_INFO("Event received: RESET\r\n");

			/* The device reset is always in Lorawan mode */
			if (lora_params.nwm == 0)
			{
				smtc_modem_test_start();
			}

			/* If you add a parameter initialization here, the test mode won't work */

			// This code needs to be automatically connected to the network, so we need to set the parameters here, regardless of the test mode
			// uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {0};
			// memset(custom_datarate, lora_params.dr, SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH);
			// smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, custom_datarate);

			// uint8_t rc = smtc_modem_set_class(stack_id, lora_params.class);
			// if (rc != SMTC_MODEM_RC_OK)
			// {
			//     SMTC_HAL_TRACE_WARNING("smtc_modem_set_class failed: rc=(%d)\r\n", rc);
			// }

			// Schedule a Join LoRaWAN network
			// smtc_modem_join_network(stack_id);
			break;

		case SMTC_MODEM_EVENT_ALARM:
			SMTC_HAL_TRACE_INFO("Event received: ALARM\r\n");

			if (lora_params.interval > 0)
				smtc_modem_alarm_start_timer(lora_params.interval);
			data_lpp_uplink();
			break;

		case SMTC_MODEM_EVENT_JOINED:
			am_util_stdio_printf("+EVT:JOINED\r\n");

			if (lora_params.interval > 0)
				smtc_modem_alarm_start_timer(lora_params.interval);

			break;

		case SMTC_MODEM_EVENT_TXDONE:
			am_util_stdio_printf("+EVT:TX_DONE\r\n");

			if(current_event.event_data.txdone.status == SMTC_MODEM_EVENT_TXDONE_CONFIRMED)
			{
				am_util_stdio_printf("+EVT:SEND_CONFIRMED_OK\r\n");
			}else
			{
				am_util_stdio_printf("+EVT:SEND_CONFIRMED_FAILED\r\n");
			}

			break;

		case SMTC_MODEM_EVENT_DOWNDATA:
			SMTC_HAL_TRACE_INFO("Event received: DOWNDATA\r\n");
			rx_payload_size = (uint8_t)current_event.event_data.downdata.length;
			memcpy(rx_payload, current_event.event_data.downdata.data, rx_payload_size);
			SMTC_HAL_TRACE_PRINTF("Data received on port %u\r\n", current_event.event_data.downdata.fport);
			SMTC_HAL_TRACE_ARRAY("Received payload", rx_payload, rx_payload_size);

		case SMTC_MODEM_EVENT_UPLOADDONE:
			SMTC_HAL_TRACE_INFO("Event received: UPLOADDONE\r\n");

			break;

		case SMTC_MODEM_EVENT_SETCONF:
			SMTC_HAL_TRACE_INFO("Event received: SETCONF\r\n");

			break;

		case SMTC_MODEM_EVENT_MUTE:
			SMTC_HAL_TRACE_INFO("Event received: MUTE\r\n");

			break;

		case SMTC_MODEM_EVENT_STREAMDONE:
			SMTC_HAL_TRACE_INFO("Event received: STREAMDONE\r\n");

			break;

		case SMTC_MODEM_EVENT_JOINFAIL:
			am_util_stdio_printf("+EVT:JOIN_FAILED_RX_TIMEOUT\r\n");
			/* Stop joining the network after failure */
			smtc_modem_leave_network(stack_id);

			break;

		case SMTC_MODEM_EVENT_TIME:
			SMTC_HAL_TRACE_INFO("Event received: TIME\r\n");
			break;

		case SMTC_MODEM_EVENT_TIMEOUT_ADR_CHANGED:
			SMTC_HAL_TRACE_INFO("Event received: TIMEOUT_ADR_CHANGED\r\n");
			break;

		case SMTC_MODEM_EVENT_NEW_LINK_ADR:
			SMTC_HAL_TRACE_INFO("Event received: NEW_LINK_ADR\r\n");
			break;

		case SMTC_MODEM_EVENT_LINK_CHECK:
			SMTC_HAL_TRACE_INFO("Event received: LINK_CHECK\r\n");
			break;

		case SMTC_MODEM_EVENT_ALMANAC_UPDATE:
			SMTC_HAL_TRACE_INFO("Event received: ALMANAC_UPDATE\r\n");
			break;

		case SMTC_MODEM_EVENT_USER_RADIO_ACCESS:
			SMTC_HAL_TRACE_INFO("Event received: USER_RADIO_ACCESS\r\n");
			break;

		case SMTC_MODEM_EVENT_CLASS_B_PING_SLOT_INFO:
			SMTC_HAL_TRACE_INFO("Event received: CLASS_B_PING_SLOT_INFO\r\n");
			break;

		case SMTC_MODEM_EVENT_CLASS_B_STATUS:
			SMTC_HAL_TRACE_INFO("Event received: CLASS_B_STATUS\r\n");
			break;

		default:
			SMTC_HAL_TRACE_ERROR("Unknown event %u\r\n", current_event.event_type);
			break;
		}
	} while (event_pending_count > 0);
}

void lorawan_init()
{
	SMTC_HAL_TRACE_INFO("RAK LoRaWAN ISM2400 Example\r\n");
	hal_spi_init(0, 0, 0, 0);
	hal_rtc_init();
	hal_lp_timer_init();
	hal_mcu_disable_irq();
	hal_mcu_init();
	smtc_modem_init(&modem_radio, &get_event);
	hal_mcu_enable_irq();
	smtc_modem_set_region(STACK_ID, MODEM_EXAMPLE_REGION);
	smtc_modem_set_tx_power_offset_db(STACK_ID, 0);

	load_lora_params();

	char *mode[2] = {"P2P", "LORAWAN"};
	am_util_stdio_printf("Work Mode %s\r\n", mode[lora_params.nwm]);

	/* ABP mode */
	if (lora_params.join_mode == 0)
	{
		/* The ABP mode pass parameter is 1 */
		lorawan_api_set_activation_mode(1);

		int ret;
		am_util_stdio_printf("ABP mode\r\n");
		lorawan_api_devaddr_set(lora_params.devaddr);
		ret = smtc_modem_crypto_set_key(SMTC_SE_NWK_S_ENC_KEY, lora_params.nwkskey);
		if (ret)
		{
			SMTC_HAL_TRACE_ERROR("SMTC_SE_NWK_S_ENC_KEY ERROR\r\n");
		}
		ret = smtc_modem_crypto_set_key(SMTC_SE_APP_S_KEY, lora_params.appskey);
		if (ret)
		{
			SMTC_HAL_TRACE_ERROR("SMTC_SE_APP_S_KEY ERROR\r\n");
		}
	}
	else
	{
		smtc_modem_set_deveui(STACK_ID, lora_params.dev_eui);
		smtc_modem_set_joineui(STACK_ID, lora_params.join_eui);
		smtc_modem_set_nwkkey(STACK_ID, lora_params.app_key);
		am_util_stdio_printf("OTAA mode\r\n");
	}

	uint8_t rc = smtc_modem_set_class(STACK_ID, lora_params.class);
	if (rc != SMTC_MODEM_RC_OK)
	{
		SMTC_HAL_TRACE_WARNING("smtc_modem_set_class failed: rc=(%d)\r\n", rc);
	}

	lorawan_api_dr_strategy_set(USER_DR_DISTRIBUTION);
	smtc_modem_set_nb_trans(STACK_ID, lora_params.retry);
}

void data_lpp_uplink()
{
	uint8_t buff_idx = 0;
	int8_t buffer[24] = {0};

	if (lis3dh_initialized == true)
	{
		RAK1904_func();
		buffer[buff_idx++] = 1;	   // channel
		buffer[buff_idx++] = 0x71; // LPP

		buffer[buff_idx++] = (val[0]) >> 8;
		buffer[buff_idx++] = val[0];

		buffer[buff_idx++] = (val[1]) >> 8;
		buffer[buff_idx++] = (val[1]);

		buffer[buff_idx++] = (val[2]) >> 8;
		buffer[buff_idx++] = (val[2]);
	}
	if (shtc3_initialized == true)
	{
		RAK1901_func();
		buffer[buff_idx++] = 2;	   // channel
		buffer[buff_idx++] = 0x67; // LPP

		buffer[buff_idx++] = (uint8_t)(val[3] >> 8);
		buffer[buff_idx++] = (uint8_t)val[3];

		buffer[buff_idx++] = 2;	   // channel
		buffer[buff_idx++] = 0x68; // LPP

		buffer[buff_idx++] = (uint8_t)(val[4]);
	}

	if (buff_idx != 0)
	{
		smtc_modem_request_uplink(STACK_ID, 1, true, buffer, buff_idx);
	}
}



static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF; // 初始化CRC寄存器

    while (length--)
    {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ *data++) & 0xFF];
    }

    return crc;
}


uint16_t calculate_crc_for_lorawan_params(const LoRaWAN_Params *params) {
    return crc16((const uint8_t *)params, sizeof(LoRaWAN_Params) - sizeof(params->crc));
}