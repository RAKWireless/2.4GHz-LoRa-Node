#ifndef I2C_H
#define I2C_H
#include "am_mcu_apollo.h"
#include "am_util.h"
#include <stdlib.h>

void iom_slave_write_offset(uint32_t offset, uint32_t *pBuf, uint32_t size);
void iom_slave_read_offset(uint32_t offset, uint32_t *pBuf, uint32_t size);

void iom_slave_read(uint32_t *pBuf, uint32_t size);
void iom_slave_write(uint32_t *pBuf, uint32_t size);

extern int16_t val[3];   // x y z mg
void RAK1904_func(void) ;
#endif