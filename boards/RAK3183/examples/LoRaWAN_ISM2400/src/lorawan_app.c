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

#include "i2c.h"

#define ADDR_FLASH_AT_PARAM_CONTEXT   (AM_HAL_FLASH_INSTANCE_SIZE + (4 * AM_HAL_FLASH_PAGE_SIZE))
#define APP_TX_DUTYCYCLE              (7)

void lis3dh_lpp_uplink(void);
/**
 * @brief LoRaWAN User credentials
 */
#define USER_LORAWAN_DEVICE_EUI     { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00}
#define USER_LORAWAN_JOIN_EUI       { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00}
#define USER_LORAWAN_APP_KEY        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, \
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00}
                                      
#define MODEM_EXAMPLE_REGION    SMTC_MODEM_REGION_WW2G4

/**
 * Stack id value (multistacks modem is not yet available)
 */
#define STACK_ID 0

/**
 * @brief Stack credentials
 */



LoRaWAN_Params lora_params = {
    .dev_eui = USER_LORAWAN_DEVICE_EUI,
    .join_eui = USER_LORAWAN_JOIN_EUI,
    .app_key = USER_LORAWAN_APP_KEY,
	  .class = 0,
		.dr    = 0
};
						
																			
uint8_t	rx_payload_size;
uint8_t rx_payload[256];														
																			
#if defined( SX128X )
const ralf_t modem_radio = RALF_SX128X_INSTANTIATE( NULL );
#elif defined( SX126X )
const ralf_t modem_radio = RALF_SX126X_INSTANTIATE( NULL );
#elif defined( LR11XX )
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE( NULL );
#else
#error "Please select radio board.."
#endif


void save_lora_params(void) {
	
     hal_flash_erase_page( ADDR_FLASH_AT_PARAM_CONTEXT, 1 );
		 hal_flash_write_buffer(ADDR_FLASH_AT_PARAM_CONTEXT , (uint8_t *)&lora_params,sizeof(lora_params)); 
	
	   uint8_t stack_id = STACK_ID;
}

void load_lora_params(void) {
    hal_flash_read_buffer(ADDR_FLASH_AT_PARAM_CONTEXT,(uint8_t *)&lora_params,sizeof(lora_params));
	
		if(lora_params.class == 255 ||lora_params.interval == 0xFFFFFFFF )
		{
			lora_params.class = 0;
			lora_params.dr = 0;  
			lora_params.interval = 0;
		}
}


static void get_event( void )
{
    smtc_modem_event_t current_event;
    uint8_t            event_pending_count;
    uint8_t            stack_id = STACK_ID;

    // Continue to read modem event until all event has been processed
    do
    {
        // Read modem event
        smtc_modem_get_event( &current_event, &event_pending_count );

        switch( current_event.event_type )
        {
        case SMTC_MODEM_EVENT_RESET:
            SMTC_HAL_TRACE_INFO( "Event received: RESET\n" );
				    /*  */
						load_lora_params();

            // Set user region
            smtc_modem_set_region( stack_id, MODEM_EXAMPLE_REGION );
									
						smtc_modem_set_deveui( stack_id, lora_params.dev_eui );
            smtc_modem_set_joineui( stack_id,lora_params.join_eui );
            smtc_modem_set_nwkkey( stack_id, lora_params.app_key );
						
				
			      //This code needs to be automatically connected to the network, so we need to set the parameters here, regardless of the test mode
				    uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {0};             
				    memset(custom_datarate,lora_params.dr,SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH);
	          smtc_modem_adr_set_profile( STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, custom_datarate );
						
						uint8_t rc = smtc_modem_set_class( stack_id, lora_params.class );
						if( rc != SMTC_MODEM_RC_OK )
						{
								SMTC_HAL_TRACE_WARNING( "smtc_modem_set_class failed: rc=(%d)\n", rc );
						}
						
            //Schedule a Join LoRaWAN network
            smtc_modem_join_network( stack_id );
            break;

        case SMTC_MODEM_EVENT_ALARM:
            SMTC_HAL_TRACE_INFO( "Event received: ALARM\n" );
				
				    if(lora_params.interval > 0)
 				    smtc_modem_alarm_start_timer( lora_params.interval );
  				  lis3dh_lpp_uplink();
            break;

        case SMTC_MODEM_EVENT_JOINED:
            SMTC_HAL_TRACE_INFO( "Event received: JOINED\n" );
            SMTC_HAL_TRACE_INFO( "Modem is now joined \n" );
				
					  if(lora_params.interval > 0)
						smtc_modem_alarm_start_timer( lora_params.interval  );
				  
				
            break;

        case SMTC_MODEM_EVENT_TXDONE:
            SMTC_HAL_TRACE_INFO( "Event received: TXDONE\n" );
            SMTC_HAL_TRACE_INFO( "Transmission done \n" );

            break;

        case SMTC_MODEM_EVENT_DOWNDATA:
            SMTC_HAL_TRACE_INFO( "Event received: DOWNDATA\n" );
            rx_payload_size = ( uint8_t ) current_event.event_data.downdata.length;
            memcpy( rx_payload, current_event.event_data.downdata.data, rx_payload_size );
            SMTC_HAL_TRACE_PRINTF( "Data received on port %u\n", current_event.event_data.downdata.fport );
            SMTC_HAL_TRACE_ARRAY( "Received payload", rx_payload, rx_payload_size );
            break;

        case SMTC_MODEM_EVENT_UPLOADDONE:
            SMTC_HAL_TRACE_INFO( "Event received: UPLOADDONE\n" );

            break;

        case SMTC_MODEM_EVENT_SETCONF:
            SMTC_HAL_TRACE_INFO( "Event received: SETCONF\n" );

            break;

        case SMTC_MODEM_EVENT_MUTE:
            SMTC_HAL_TRACE_INFO( "Event received: MUTE\n" );

            break;

        case SMTC_MODEM_EVENT_STREAMDONE:
            SMTC_HAL_TRACE_INFO( "Event received: STREAMDONE\n" );

            break;

        case SMTC_MODEM_EVENT_JOINFAIL:
            SMTC_HAL_TRACE_INFO( "Event received: JOINFAIL\n" );
            SMTC_HAL_TRACE_WARNING( "Join request failed \n" );
						/* Stop joining the network after failure */
				    smtc_modem_leave_network(stack_id);
				    
            break;

        case SMTC_MODEM_EVENT_TIME:
            SMTC_HAL_TRACE_INFO( "Event received: TIME\n" );
            break;

        case SMTC_MODEM_EVENT_TIMEOUT_ADR_CHANGED:
            SMTC_HAL_TRACE_INFO( "Event received: TIMEOUT_ADR_CHANGED\n" );
            break;

        case SMTC_MODEM_EVENT_NEW_LINK_ADR:
            SMTC_HAL_TRACE_INFO( "Event received: NEW_LINK_ADR\n" );
            break;

        case SMTC_MODEM_EVENT_LINK_CHECK:
            SMTC_HAL_TRACE_INFO( "Event received: LINK_CHECK\n" );
            break;

        case SMTC_MODEM_EVENT_ALMANAC_UPDATE:
            SMTC_HAL_TRACE_INFO( "Event received: ALMANAC_UPDATE\n" );
            break;

        case SMTC_MODEM_EVENT_USER_RADIO_ACCESS:
            SMTC_HAL_TRACE_INFO( "Event received: USER_RADIO_ACCESS\n" );
            break;

        case SMTC_MODEM_EVENT_CLASS_B_PING_SLOT_INFO:
            SMTC_HAL_TRACE_INFO( "Event received: CLASS_B_PING_SLOT_INFO\n" );
            break;

        case SMTC_MODEM_EVENT_CLASS_B_STATUS:
            SMTC_HAL_TRACE_INFO( "Event received: CLASS_B_STATUS\n" );
            break;

        default:
            SMTC_HAL_TRACE_ERROR( "Unknown event %u\n", current_event.event_type );
            break;
        }
    } while( event_pending_count > 0 );
}

void lorawan_init()
{
		SMTC_HAL_TRACE_INFO( "RAK LoRaWAN ISM2400 Example\n" );
  	hal_spi_init(0,0,0,0);
	  hal_rtc_init(  );
	  hal_lp_timer_init();
		hal_mcu_disable_irq( );   
		hal_mcu_init( ); 
		smtc_modem_init( &modem_radio, &get_event );
		hal_mcu_enable_irq( );
		smtc_modem_set_region( STACK_ID, MODEM_EXAMPLE_REGION );
	  smtc_modem_set_tx_power_offset_db( STACK_ID, 0 );

}

void lis3dh_lpp_uplink()
{
		RAK1904_func();	
		int8_t buffer[8]={0};
		buffer[0] = 1;      //channel
		buffer[1] = 0x71 ;  //LPP  
				 
		buffer[2] = (val[0])>>8;
		buffer[3] =  val[0];
		
		buffer[4] = (val[1])>>8;
		buffer[5] = (val[1]);
		
		buffer[6] = (val[2])>>8;
		buffer[7] = (val[2]);
			
		smtc_modem_request_uplink( STACK_ID, 1, true, buffer, sizeof(buffer) ) ;
}

