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
    .join_mode = 0,
};

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

	hal_flash_erase_page(ADDR_FLASH_AT_PARAM_CONTEXT, 1);
	hal_flash_write_buffer(ADDR_FLASH_AT_PARAM_CONTEXT, (uint8_t *)&lora_params, sizeof(lora_params));

	uint8_t stack_id = STACK_ID;
}

void load_lora_params(void)
{
	hal_flash_read_buffer(ADDR_FLASH_AT_PARAM_CONTEXT, (uint8_t *)&lora_params, sizeof(lora_params));

    if (lora_params.join_mode == 0xFF)
    {
        /* Setting default parameters  todo: adding the crc parameter */
        lora_params.devaddr = 0;
        lora_params.class = 0;
        lora_params.dr = 0;
        lora_params.confirm = 1;
        lora_params.retry = 1;
        lora_params.join_mode = 0;
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
            SMTC_HAL_TRACE_INFO("Event received: RESET\n");
            /* If you add a parameter initialization here, the test mode won't work */

			// This code needs to be automatically connected to the network, so we need to set the parameters here, regardless of the test mode
			// uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {0};
			// memset(custom_datarate, lora_params.dr, SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH);
			// smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, custom_datarate);

			// uint8_t rc = smtc_modem_set_class(stack_id, lora_params.class);
			// if (rc != SMTC_MODEM_RC_OK)
			// {
			//     SMTC_HAL_TRACE_WARNING("smtc_modem_set_class failed: rc=(%d)\n", rc);
			// }

			// Schedule a Join LoRaWAN network
			// smtc_modem_join_network(stack_id);
			break;

		case SMTC_MODEM_EVENT_ALARM:
			SMTC_HAL_TRACE_INFO("Event received: ALARM\n");

			if (lora_params.interval > 0)
				smtc_modem_alarm_start_timer(lora_params.interval);
			data_lpp_uplink();
			break;

		case SMTC_MODEM_EVENT_JOINED:
			SMTC_HAL_TRACE_INFO("Event received: JOINED\n");
			SMTC_HAL_TRACE_INFO("Modem is now joined \n");

			if (lora_params.interval > 0)
				smtc_modem_alarm_start_timer(lora_params.interval);

			break;

		case SMTC_MODEM_EVENT_TXDONE:
			SMTC_HAL_TRACE_INFO("Event received: TXDONE\n");
			SMTC_HAL_TRACE_INFO("Transmission done \n");

			break;

        case SMTC_MODEM_EVENT_DOWNDATA:
            SMTC_HAL_TRACE_INFO("Event received: DOWNDATA\n");
            rx_payload_size = (uint8_t)current_event.event_data.downdata.length;
            memcpy(rx_payload, current_event.event_data.downdata.data, rx_payload_size);
            SMTC_HAL_TRACE_PRINTF("Data received on port %u\n", current_event.event_data.downdata.fport);
            SMTC_HAL_TRACE_ARRAY("Received payload", rx_payload, rx_payload_size);


		case SMTC_MODEM_EVENT_UPLOADDONE:
			SMTC_HAL_TRACE_INFO("Event received: UPLOADDONE\n");

			break;

		case SMTC_MODEM_EVENT_SETCONF:
			SMTC_HAL_TRACE_INFO("Event received: SETCONF\n");

			break;

		case SMTC_MODEM_EVENT_MUTE:
			SMTC_HAL_TRACE_INFO("Event received: MUTE\n");

			break;

		case SMTC_MODEM_EVENT_STREAMDONE:
			SMTC_HAL_TRACE_INFO("Event received: STREAMDONE\n");

			break;

		case SMTC_MODEM_EVENT_JOINFAIL:
			SMTC_HAL_TRACE_INFO("Event received: JOINFAIL\n");
			SMTC_HAL_TRACE_WARNING("Join request failed \n");
			/* Stop joining the network after failure */
			smtc_modem_leave_network(stack_id);

			break;

		case SMTC_MODEM_EVENT_TIME:
			SMTC_HAL_TRACE_INFO("Event received: TIME\n");
			break;

		case SMTC_MODEM_EVENT_TIMEOUT_ADR_CHANGED:
			SMTC_HAL_TRACE_INFO("Event received: TIMEOUT_ADR_CHANGED\n");
			break;

		case SMTC_MODEM_EVENT_NEW_LINK_ADR:
			SMTC_HAL_TRACE_INFO("Event received: NEW_LINK_ADR\n");
			break;

		case SMTC_MODEM_EVENT_LINK_CHECK:
			SMTC_HAL_TRACE_INFO("Event received: LINK_CHECK\n");
			break;

		case SMTC_MODEM_EVENT_ALMANAC_UPDATE:
			SMTC_HAL_TRACE_INFO("Event received: ALMANAC_UPDATE\n");
			break;

		case SMTC_MODEM_EVENT_USER_RADIO_ACCESS:
			SMTC_HAL_TRACE_INFO("Event received: USER_RADIO_ACCESS\n");
			break;

		case SMTC_MODEM_EVENT_CLASS_B_PING_SLOT_INFO:
			SMTC_HAL_TRACE_INFO("Event received: CLASS_B_PING_SLOT_INFO\n");
			break;

		case SMTC_MODEM_EVENT_CLASS_B_STATUS:
			SMTC_HAL_TRACE_INFO("Event received: CLASS_B_STATUS\n");
			break;

		default:
			SMTC_HAL_TRACE_ERROR("Unknown event %u\n", current_event.event_type);
			break;
		}
	} while (event_pending_count > 0);
}

void lorawan_init()
{
    SMTC_HAL_TRACE_INFO("RAK LoRaWAN ISM2400 Example\n");
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

    smtc_modem_set_deveui(STACK_ID, lora_params.dev_eui);
    smtc_modem_set_joineui(STACK_ID, lora_params.join_eui);
    smtc_modem_set_nwkkey(STACK_ID, lora_params.app_key);

    lorawan_api_set_activation_mode(lora_params.join_mode);

    /* ABP mode */
    if(lora_params.join_mode == 1)
    {
        int ret ;
        SMTC_HAL_TRACE_INFO("ABP mode\r\n");
        lorawan_api_devaddr_set(lora_params.devaddr);
        ret = smtc_modem_crypto_set_key(SMTC_SE_NWK_S_ENC_KEY,lora_params.nwkskey);
        if(ret)
        {
            SMTC_HAL_TRACE_ERROR("SMTC_SE_NWK_S_ENC_KEY ERROR\r\n");
        }
        ret = smtc_modem_crypto_set_key(SMTC_SE_APP_S_KEY,lora_params.appskey);
        if(ret)
        {
            SMTC_HAL_TRACE_ERROR("SMTC_SE_APP_S_KEY ERROR\r\n");
        }
    }

    uint8_t rc = smtc_modem_set_class(STACK_ID, lora_params.class);
    if (rc != SMTC_MODEM_RC_OK)
    {
        SMTC_HAL_TRACE_WARNING("smtc_modem_set_class failed: rc=(%d)\n", rc);
    }

    lorawan_api_dr_strategy_set(USER_DR_DISTRIBUTION);
    smtc_modem_set_nb_trans(STACK_ID,lora_params.retry);
    
            
}

void data_lpp_uplink()
{
	uint8_t buff_idx = 0;
	int8_t buffer[24] = {0};

	if (lis3dh_initialized == true)
	{
		RAK1904_func();
		buffer[buff_idx++] = 1; // channel
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
		buffer[buff_idx++] = 2; // channel
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
