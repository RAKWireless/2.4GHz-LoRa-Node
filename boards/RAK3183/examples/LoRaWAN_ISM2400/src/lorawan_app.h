#ifndef TX_RX_TEST_H
#define TX_RX_TEST_H
#include "am_mcu_apollo.h"

uint16_t crc16(const uint8_t *data, size_t length);

typedef struct
{
	uint8_t dev_eui[8];
	uint8_t join_eui[8];
	uint8_t app_key[16];
	
	uint8_t nwkskey[16];
	uint8_t appskey[16];

	uint32_t devaddr;

	uint8_t class;
	uint8_t dr;
	uint8_t confirm;
	uint8_t retry;

	uint32_t interval; 

	uint8_t join_mode;
	uint8_t nwm;
	uint8_t range_test_enabled;  // Range test mode (0=disabled, 1=enabled) 
	uint8_t tx_power_dbm;

	uint32_t frequency_hz;

	uint8_t sf;
	uint8_t bw;
	uint8_t cr;
	uint8_t preamble_size;

	uint32_t auto_send_interval_sec; // 自动发送间隔（秒）

	uint16_t temp;
	uint16_t crc;      //结尾一定要4字节对齐
} LoRaWAN_Params;

extern volatile LoRaWAN_Params lora_params;

#endif