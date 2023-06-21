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
//#include "smtc_board.h"
//#include "smtc_hal.h"
//#include "apps_modem_common.h"
//#include "apps_modem_event.h"
#include "smtc_modem_test_api.h"
//#include "smtc_board_ralf.h"
//#include "apps_utilities.h"
#include "smtc_modem_utilities.h"


/**
 * @brief LoRaWAN User credentials
 */
#define USER_LORAWAN_DEVICE_EUI     { 0x99, 0x11, 0x11, 0x88, 0x88, 0x00, 0x00, 0x00 }
#define USER_LORAWAN_JOIN_EUI       { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define USER_LORAWAN_APP_KEY        { 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
                                      
#define MODEM_EXAMPLE_REGION    SMTC_MODEM_REGION_WW2G4

/**
 * Stack id value (multistacks modem is not yet available)
 */
#define STACK_ID 0

/**
 * @brief Stack credentials
 */
static const uint8_t user_dev_eui[8]  = USER_LORAWAN_DEVICE_EUI;
static const uint8_t user_join_eui[8] = USER_LORAWAN_JOIN_EUI;
static const uint8_t user_app_key[16] = USER_LORAWAN_APP_KEY;

#if defined( SX128X )
const ralf_t modem_radio = RALF_SX128X_INSTANTIATE( NULL );
#elif defined( SX126X )
const ralf_t modem_radio = RALF_SX126X_INSTANTIATE( NULL );
#elif defined( LR11XX )
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE( NULL );
#else
#error "Please select radio board.."
#endif


#define RADIO_NSS               11      //NSS 还是有作用的


#define RADIO_NRST              17     //复位脚

#define RADIO_DIOX              15      //查到代码 应该是DIO1

#define RADIO_BUSY_PIN          16      //busy 脚  16

#define RADIO_SPI_ID            1      //这个也是传入无效值  底层是写的一个SPI

#define TXCO_POWER              2

static void get_event( void )
{
    //SMTC_HAL_TRACE_MSG_COLOR( "get_event () callback\n", HAL_DBG_TRACE_COLOR_BLUE );

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

            // Set user credentials
            smtc_modem_set_deveui( stack_id, user_dev_eui );
            smtc_modem_set_joineui( stack_id, user_join_eui );
            smtc_modem_set_nwkkey( stack_id, user_app_key );
            // Set user region
            smtc_modem_set_region( stack_id, MODEM_EXAMPLE_REGION );
            // Schedule a Join LoRaWAN network
            smtc_modem_join_network( stack_id );
            break;

        case SMTC_MODEM_EVENT_ALARM:
            SMTC_HAL_TRACE_INFO( "Event received: ALARM\n" );

            break;

        case SMTC_MODEM_EVENT_JOINED:
            SMTC_HAL_TRACE_INFO( "Event received: JOINED\n" );
            SMTC_HAL_TRACE_INFO( "Modem is now joined \n" );
            break;

        case SMTC_MODEM_EVENT_TXDONE:
            SMTC_HAL_TRACE_INFO( "Event received: TXDONE\n" );
            SMTC_HAL_TRACE_INFO( "Transmission done \n" );
            break;

        case SMTC_MODEM_EVENT_DOWNDATA:
            SMTC_HAL_TRACE_INFO( "Event received: DOWNDATA\n" );
//            rx_payload_size = ( uint8_t ) current_event.event_data.downdata.length;
//            memcpy( rx_payload, current_event.event_data.downdata.data, rx_payload_size );
//            SMTC_HAL_TRACE_PRINTF( "Data received on port %u\n", current_event.event_data.downdata.fport );
//            SMTC_HAL_TRACE_ARRAY( "Received payload", rx_payload, rx_payload_size );
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

void test_radio()
{
//	uint8_t buffer;
//	
//	hal_mcu_init();
//	
//	void *context;
//	sx128x_hal_reset( context );    //SPI 读测试通了     //写测试有问题
//	
  	hal_spi_init(1,1,1,0);
	
	  hal_rtc_init(  );
	
	  hal_lp_timer_init();
//	sx128x_read_register(context,0x0891,&buffer,1);
//	am_util_stdio_printf("buffer  %02X\r\n",buffer);
//	
//	sx128x_read_register(context,0x089F,&buffer,1);
//	am_util_stdio_printf("buffer  %02X\r\n",buffer);
//	
//	buffer = 0x55;
//	
//	sx128x_write_register(context,0x0891,&buffer,1);
//	sx128x_read_register(context,0x0891,&buffer,1);
//	am_util_stdio_printf("buffer  %02X",buffer);
	
//	smtc_board_initialise_and_get_ralf

		 hal_mcu_disable_irq( );   //这里没有关中断

		 hal_mcu_init( ); 
		 am_hal_gpio_pinconfig(RADIO_BUSY_PIN, g_AM_HAL_GPIO_INPUT);
		 smtc_modem_init( &modem_radio, &get_event );
		  
		 hal_mcu_enable_irq( );
		 
		 
		 
//		 while(1)
//		 {
//			  hal_gpio_set_value( RADIO_NRST, 0 );
//			  hal_mcu_wait_us( 5000 );
//			 
//			if( hal_gpio_get_value( RADIO_BUSY_PIN ) == 1 )
//			{
//					am_util_stdio_printf("sx128x_hal_wait_on_busy\n");
//					hal_gpio_init_out(44 ,1 );
//			}
//			 am_util_delay_ms( 10 );
//	
//			 hal_gpio_set_value( RADIO_NRST, 1 );
//			 am_util_delay_ms( 1000 );
//			  
//		   //sx128x_hal_reset();
//		 }
//     SMTC_HAL_TRACE_INFO( "EXTI example is starting.\n");
     //SMTC_HAL_TRACE_INFO( "EXTI example is starting. hal_rng_get_random %d\n",hal_rng_get_random() );
		 
		 //am_util_delay_ms(1000);
		 
	   //test();
		 
		 //smtc_modem_hal_reset_mcu();
	
}


//void hal_spi_init( const uint32_t id, const hal_gpio_pin_names_t mosi, const hal_gpio_pin_names_t miso,
//                   const hal_gpio_pin_names_t sclk )


//sx128x_status_t sx128x_read_register( const void* context, const uint16_t address, uint8_t* buffer,
//                                      const uint16_t size )
//{
//    const uint8_t buf[SX128X_SIZE_READ_REGISTER] = {
//        SX128X_READ_REGISTER,
//        ( uint8_t )( address >> 8 ),
//        ( uint8_t )( address >> 0 ),
//        SX128X_NOP,
//    };

//    return ( sx128x_status_t ) sx128x_hal_read( context, buf, SX128X_SIZE_READ_REGISTER, buffer, size );
//}

//sx128x_status_t sx128x_write_register( const void* context, const uint16_t address, const uint8_t* buffer,
//                                       const uint16_t size )
//{
//    const uint8_t buf[SX128X_SIZE_WRITE_REGISTER] = {
//        SX128X_WRITE_REGISTER,
//        ( uint8_t )( address >> 8 ),
//        ( uint8_t )( address >> 0 ),
//    };

//    return ( sx128x_status_t ) sx128x_hal_write( context, buf, SX128X_SIZE_WRITE_REGISTER, buffer, size );
//}