#ifndef TX_RX_TEST_H
#define TX_RX_TEST_H
#include "am_mcu_apollo.h"

typedef struct {
    uint8_t dev_eui[8];
    uint8_t join_eui[8];
    uint8_t app_key[16];
		uint8_t class;
	  uint8_t dr;
	  uint16_t temp;
	  uint32_t interval;
} LoRaWAN_Params;

extern LoRaWAN_Params lora_params;


#endif