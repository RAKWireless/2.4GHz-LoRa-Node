#ifndef TX_RX_TEST_H
#define TX_RX_TEST_H
#include "am_mcu_apollo.h"

uint16_t crc16(const uint8_t *data, size_t length);

typedef struct
{
	uint8_t dev_eui[8];
	uint8_t join_eui[8];
	uint8_t app_key[16];
	uint32_t devaddr;
	uint8_t nwkskey[16];
	uint8_t appskey[16];
	uint8_t class;
	uint8_t dr;
	uint8_t confirm;
	uint32_t interval; 
	uint8_t retry;
	uint8_t join_mode;

	uint8_t nwm; 
	uint32_t frequency_hz;
	uint8_t tx_power_dbm;
	uint8_t sf;
	uint8_t bw;
	uint8_t cr;
	uint8_t preamble_size;
	uint16_t crc;
} LoRaWAN_Params;

extern LoRaWAN_Params lora_params;

#endif