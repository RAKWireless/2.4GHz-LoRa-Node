/**
 * @file rak1904.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes and definitions for RAK1904
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef RAK1904_H
#define RAK1904_H
#include "am_mcu_apollo.h"
#include "am_util.h"
#include <stdlib.h>
#include "i2c.h"

void RAK1904(void);

// LIS3DH�Ĵ�������

#define LIS3DH_I2C_ADDR 0x18U
#define LIS3DH_WHO_AM_I 0x0FU
#define LIS3DH_CTRL_REG4 0x23U
#define LIS3DH_CTRL_REG1 0x20U
#define LIS3DH_CTRL_REG0 0x1EU
#define LIS3DH_STATUS_REG 0x27U

#define LIS3DH_OUT_X_L 0x28U
#define LIS3DH_OUT_X_H 0x29U
#define LIS3DH_OUT_Y_L 0x2AU
#define LIS3DH_OUT_Y_H 0x2BU
#define LIS3DH_OUT_Z_L 0x2CU
#define LIS3DH_OUT_Z_H 0x2DU

int32_t lis3dh_block_data_update_set(void);
int32_t lis3dh_data_rate_set(void);
int32_t lis3dh_operating_mode_set(void);
int32_t lis3dh_full_scale_set(void);
int32_t lis3dh_acceleration_raw_get(void);

#endif // RAK1904_H