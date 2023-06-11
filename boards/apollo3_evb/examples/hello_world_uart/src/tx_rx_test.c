#include "tx_rx_test.h"

#include "am_mcu_apollo.h"
#include "am_util.h"


//#include "smtc_board.h"
//#include "smtc_hal.h"
//#include "apps_modem_common.h"
//#include "apps_modem_event.h"
#include "smtc_modem_test_api.h"
//#include "smtc_board_ralf.h"
//#include "apps_utilities.h"
#include "smtc_modem_utilities.h"


void test_radio()
{
	uint8_t buffer;
	
	hal_mcu_init();
	
	void *context;
	sx128x_hal_reset( context );    //SPI 读测试通了     //写测试有问题
	
	hal_spi_init(1,1,1,0);
	sx128x_read_register(context,0x0891,&buffer,1);
	am_util_stdio_printf("buffer  %02X\r\n",buffer);
	
	sx128x_read_register(context,0x089F,&buffer,1);
	am_util_stdio_printf("buffer  %02X\r\n",buffer);
	
	buffer = 0x55;
	
	sx128x_write_register(context,0x0891,&buffer,1);
	sx128x_read_register(context,0x0891,&buffer,1);
	am_util_stdio_printf("buffer  %02X",buffer);
	
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