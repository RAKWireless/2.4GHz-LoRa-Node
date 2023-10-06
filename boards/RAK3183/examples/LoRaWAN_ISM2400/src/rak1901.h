/**
 * @file rak1901.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes and definitions for RAK1901 -- based on Sercan Erats https://github.com/srcnert/SHTC3
 * @version 0.1
 * @version 0.1
 * @date 2023-09-19
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef RAK1901_H
#define RAK1901_H
#include "am_mcu_apollo.h"
#include "am_util.h"
#include <stdlib.h>
#include <stdint.h>
#include "i2c.h"

void RAK1901(void);

// SHTC3

//------------------------------------------------------------------------------
// Original code from Sercan Erat https://github.com/srcnert/SHTC3
//------------------------------------------------------------------------------

#define CRC_POLYNOMIAL 0x31 // P(x) = x^8 + x^5 + x^4 + 1 = 100110001

typedef uint32_t etError;

enum sht3cError_e
{
	NO_ERROR = 0,
	I2C_ERROR = 1,
	CHECKSUM_ERROR = 2,
};

typedef uint32_t (*comm_fptr_t)(uint8_t slave_address, uint8_t *p_data, uint32_t length);
typedef void (*delay_fptr_t)(uint32_t us);

typedef struct shtc3_dev_s
{
	/*! Device Id */
	uint8_t dev_id;
	/*! Bus read function pointer */
	comm_fptr_t i2c_read;
	/*! Bus write function pointer */
	comm_fptr_t i2c_write;
	/*! delay function pointer */
	delay_fptr_t delay_us;
} shtc3_dev_t;

#define delay_ms(ms) am_hal_flash_delay(FLASH_CYCLES_US(1000 * (ms)))

//==============================================================================
void SHTC3_Init(shtc3_dev_t shtc3_dev);
//==============================================================================
// Initializes the I2C bus for communication with the sensor.
//------------------------------------------------------------------------------

//==============================================================================
etError SHTC3_GetId(uint16_t *id);
//==============================================================================
// Gets the ID from the sensor.
//------------------------------------------------------------------------------
// input:  *id          pointer to a integer, where the id will be stored
//
// return: error:       I2C_ERROR      = no acknowledgment from sensor
//                      CHECKSUM_ERROR = checksum mismatch
//                      NO_ERROR       = no error

//==============================================================================
etError SHTC3_GetTempAndHumi(float *temp, float *humi);
//==============================================================================
// Gets the temperature [C] and the humidity [%RH].
//------------------------------------------------------------------------------
// input:  *temp        pointer to a floating point value, where the calculated
//                      temperature will be stored
//         *humi        pointer to a floating point value, where the calculated
//                      humidity will be stored
//
// return: error:       I2C_ERROR      = no acknowledgment from sensor
//                      CHECKSUM_ERROR = checksum mismatch
//                      NO_ERROR       = no error
//
// remark: If you use this function, then the sensor blocks the I2C-bus with
//         clock stretching during the measurement.

etError SHTC3_GetTempAndHumiRaw(uint16_t *temp, uint16_t *humi);
//==============================================================================
// Gets the temperature [C] and the humidity [%RH].
//------------------------------------------------------------------------------
// input:  *temp        pointer to a unsigned integer value, where the raw
//                      temperature will be stored
//         *humi        pointer to a unsigned integer value, where the raw
//                      humidity will be stored
//
// return: error:       I2C_ERROR      = no acknowledgment from sensor
//                      CHECKSUM_ERROR = checksum mismatch
//                      NO_ERROR       = no error
//
// remark: If you use this function, then the sensor blocks the I2C-bus with
//         clock stretching during the measurement.

//==============================================================================
etError SHTC3_GetTempAndHumiPolling(float *temp, float *humi);
//==============================================================================
// Gets the temperature [C] and the humidity [%RH]. This function polls every
// 1ms until measurement is ready.
//------------------------------------------------------------------------------
// input:  *temp        pointer to a floating point value, where the calculated
//                      temperature will be stored
//         *humi        pointer to a floating point value, where the calculated
//                      humidity will be stored
//
// return: error:       I2C_ERROR      = no acknowledgment from sensor
//                      CHECKSUM_ERROR = checksum mismatch
//                      NO_ERROR       = no error

//==============================================================================
etError SHTC3_GetTempAndHumiPollingRaw(uint16_t *temp, uint16_t *humi);
//==============================================================================
// Gets the temperature [C] and the humidity [%RH]. This function polls every
// 1ms until measurement is ready.
//------------------------------------------------------------------------------
// input:  *temp        pointer to a unsigned integer value, where the raw
//                      temperature will be stored
//         *humi        pointer to a unsigned integer value, where the raw
//                      humidity will be stored
//
// return: error:       I2C_ERROR      = no acknowledgment from sensor
//                      CHECKSUM_ERROR = checksum mismatch
//                      NO_ERROR       = no error

//==============================================================================
etError SHTC3_Sleep(void);
//==============================================================================
// Calls the function to sleep the sensor
//------------------------------------------------------------------------------
// return: error:       I2C_ERROR      = no acknowledgment from sensor
//                      CHECKSUM_ERROR = checksum mismatch
//                      NO_ERROR       = no error

//==============================================================================
etError SHTC3_Wakeup(void);
//==============================================================================
// Calls the function to wake up the sensor
//------------------------------------------------------------------------------
// return: error:       I2C_ERROR      = no acknowledgment from sensor
//                      CHECKSUM_ERROR = checksum mismatch
//                      NO_ERROR       = no error

//==============================================================================
etError SHTC3_SoftReset(void);
//==============================================================================
// Calls the soft reset mechanism that forces the sensor into a well-defined
// state without removing the power supply.
//------------------------------------------------------------------------------
// return: error:       I2C_ERROR      = no acknowledgment from sensor
//                      NO_ERROR       = no error

#endif // RAK1901_H