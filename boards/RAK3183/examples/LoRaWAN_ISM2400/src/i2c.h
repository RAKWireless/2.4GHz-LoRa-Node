#ifndef I2C_H
#define I2C_H
#include "am_mcu_apollo.h"
#include "am_util.h"
#include <stdlib.h>

bool iom_slave_write_offset(uint32_t offset, uint32_t *pBuf, uint32_t size, uint8_t i2c_addr);
bool iom_slave_read_offset(uint32_t offset, uint32_t *pBuf, uint32_t size, uint8_t i2c_addr);

bool iom_slave_read(uint32_t *pBuf, uint32_t size, uint8_t i2c_addr);
bool iom_slave_write(uint32_t *pBuf, uint32_t size, uint8_t i2c_addr);

extern int16_t val[10]; // x y z mg Temp Humidity
extern uint8_t lis3dh_initialized;
extern uint8_t shtc3_initialized;

void RAK1904_func(void);
void RAK1901_func(void);
#endif