/**
 * @file rak1901.c
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Init and read functions for RAK1901 -- based on Sercan Erats https://github.com/srcnert/SHTC3
 * @version 0.1
 * @date 2023-09-19
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "rak1901.h"

uint8_t shtc3_initialized = false;

typedef enum
{
	READ_ID = 0xEFC8,			 // command: read ID register
	SOFT_RESET = 0x805D,		 // soft reset
	SLEEP = 0xB098,				 // sleep
	WAKEUP = 0x3517,			 // wakeup
	MEAS_T_RH_POLLING = 0x7866,	 // meas. read T first, clock stretching disabled (normal mode)
	MEAS_T_RH_CLOCKSTR = 0x7CA2, // meas. read T first, clock stretching enabled (normal mode)
	MEAS_RH_T_POLLING = 0x58E0,	 // meas. read RH first, clock stretching disabled (normal mode)
	MEAS_RH_T_CLOCKSTR = 0x5C24, // meas. read RH first, clock stretching enabled (normal mode)
								 //  MEAS_T_RH_POLLING_LP  = 0x609C, // meas. read T first, clock stretching disabled (low power mode)
								 //  MEAS_T_RH_CLOCKSTR_LP = 0x6458, // meas. read T first, clock stretching enabled (low power mode)
								 //  MEAS_RH_T_POLLING_LP  = 0x401A, // meas. read RH first, clock stretching disabled (low power mode)
								 //  MEAS_RH_T_CLOCKSTR_LP = 0x44DE  // meas. read RH first, clock stretching enabled (low power mode)
} etCommands;

static etError SHTC3_Read2BytesAndCrc(uint16_t *data);
static etError SHTC3_Read4BytesAndCrc(uint16_t *data1, uint16_t *data2);
static etError SHTC3_WriteCommand(etCommands cmd);
static etError SHTC3_CheckCrc(uint8_t data[], uint8_t nbrOfBytes,
							  uint8_t checksum);
static float SHTC3_CalcTemperature(uint16_t rawValue);
static float SHTC3_CalcHumidity(uint16_t rawValue);

static uint8_t _Address = 0x70;

/**
 * @brief Translate I2C write function to Apollo3 SDK
 *
 * @param slave_address I2C address
 * @param p_data data buffer
 * @param length length of data buffer
 * @return uint32_t etError 
 */
uint32_t my_i2c_write(uint8_t slave_address, uint8_t *p_data, uint32_t length)
{
	if (!iom_slave_write((uint32_t *)p_data, length, slave_address))
	{
		return I2C_ERROR;
	}
	return NO_ERROR;
}

/**
 * @brief Translate I2C read function to Apollo3 SDK
 *
 * @param slave_address I2C address
 * @param p_data data buffer
 * @param length length of data buffer
 * @return uint32_t etError
 */
uint32_t my_i2c_read(uint8_t slave_address, uint8_t *p_data, uint32_t length)
{
	if (!iom_slave_read((uint32_t *)p_data, length, slave_address))
	{
		return I2C_ERROR;
	}
	return NO_ERROR;
}

/**
 * @brief Initialize RAK1901
 *        Sets `shtc3_initialized` to true if successful
 *
 */
void RAK1901(void)
{
	// initialize sensor module with the i2c address 0x70
	etError error;		  // error code
	uint16_t id;		  // sensor ID
	float temperature; // temperature
	float humidity;	  // relative humidity

	shtc3_dev_t shtc3_dev;
	shtc3_dev.dev_id = 0x70;
	shtc3_dev.i2c_write = &my_i2c_write;
	shtc3_dev.i2c_read = &my_i2c_read;
	shtc3_dev.delay_us = &am_hal_flash_delay;

	SHTC3_Init(shtc3_dev);

	// wake up the sensor from sleep mode
	error = SHTC3_Wakeup();
	if (error != NO_ERROR)
	{
		//am_util_stdio_printf("RAK1901 wakeup error\r\n");
		return;
	}

	am_util_stdio_printf("RAK1901 init\r\n");

	// demonstration of SoftReset command
	error = SHTC3_SoftReset();
	if (error != NO_ERROR)
	{
		am_util_stdio_printf("RAK1901 soft reset error\r\n");
		return;
	}

	// wait for sensor to reset
	delay_ms(100);

	// demonstration of GetId command
	error = SHTC3_GetId(&id);
	if (error == NO_ERROR)
	{
		am_util_stdio_printf("RAK1901 ID 0x%02X\r\n", id);
	}
	else
	{
		am_util_stdio_printf("RAK1901 get ID error\r\n");
		return;
	}

	// ID Check
	if ((id & 0x083F) == 0x0807) // Checking the form of the ID
	{							 // Bits 11 and 5-0 must match
								 // SHTC3 CORRECT ID
	}

	am_util_stdio_printf("RAK1901 wake up\r\n");

	// wake up the sensor from sleep mode
	error = SHTC3_Wakeup();
	if (error != NO_ERROR)
	{
		am_util_stdio_printf("RAK1901 wakeup error\r\n");
	}

	// Wait some time
	delay_ms(3000);

	am_util_stdio_printf("RAK1901 wake up\r\n");

	// Start Measurement
	error = SHTC3_GetTempAndHumi(&temperature, &humidity);
	if (error == NO_ERROR)
	{
		am_util_stdio_printf("SHTC3 TEMP: %f\r\n", temperature);
		am_util_stdio_printf("SHTC3 HUM: %f\r\n", humidity);
	}
	else
	{
		am_util_stdio_printf("SHTC3 COMM ERROR: %s\r\n", error == I2C_ERROR ? "I2C_ERROR" : "CHECKSUM_ERROR");
		return;
	}

	// activate the sleep mode of the sensor to save energy
	error = SHTC3_Sleep();
	if (error != NO_ERROR)
		return;

	shtc3_initialized = true;
	return;
}

/**
 * @brief Read RAK1901 sensor values
 *        Saves the values in `val[]` array at index 3 and 4
 * 
 */
void RAK1901_func(void)
{
	etError error = 0;		  // error code
	float temperature = 0; // temperature
	float humidity = 0;	  // relative humidity

	// wake up the sensor from sleep mode
	error = SHTC3_Wakeup();
	if (error != NO_ERROR)
		return;

	// Wait some time
	delay_ms(3000);

	am_util_stdio_printf("RAK1901 wake up\r\n");

	// Start Measurement
	error = SHTC3_GetTempAndHumi(&temperature, &humidity);
	if (error != NO_ERROR)
		return;

	am_util_stdio_printf("  ******************************************************************\r\n");
	am_util_stdio_printf("SHTC3 TEMP: %f\r\n", temperature);
	am_util_stdio_printf("SHTC3 HUM: %f\r\n", humidity);
	am_util_stdio_printf("  ******************************************************************\r\n");

	val[3] = (int16_t)(temperature * 10);
	val[4] = (int16_t)(humidity * 2);

	// activate the sleep mode of the sensor to save energy
	error = SHTC3_Sleep();
	if (error != NO_ERROR)
		return;
}

//------------------------------------------------------------------------------
// Original code from Sercan Erat https://github.com/srcnert/SHTC3
//------------------------------------------------------------------------------
shtc3_dev_t _shtc3_dev;
void SHTC3_Init(shtc3_dev_t shtc3_dev)
{
	_shtc3_dev = shtc3_dev;
}

//------------------------------------------------------------------------------
etError SHTC3_GetTempAndHumi(float *temp, float *humi)
{
	etError error;		   // error code
	uint16_t rawValueTemp; // temperature raw value from sensor
	uint16_t rawValueHumi; // humidity raw value from sensor

	// measure, read temperature first, clock streching enabled
	error = SHTC3_WriteCommand(MEAS_T_RH_CLOCKSTR);

	// if no error, read temperature and humidity raw values
	if (error == NO_ERROR)
	{
		error |= SHTC3_Read4BytesAndCrc(&rawValueTemp, &rawValueHumi);
	}

	// if no error, calculate temperature in C and humidity in %RH
	if (error == NO_ERROR)
	{
		*temp = SHTC3_CalcTemperature(rawValueTemp);
		*humi = SHTC3_CalcHumidity(rawValueHumi);
	}

	return error;
}

//------------------------------------------------------------------------------
etError SHTC3_GetTempAndHumiRaw(uint16_t *temp, uint16_t *humi)
{
	etError error; // error code

	// measure, read temperature first, clock streching enabled
	error = SHTC3_WriteCommand(MEAS_T_RH_CLOCKSTR);

	// if no error, read temperature and humidity raw values
	if (error == NO_ERROR)
	{
		error |= SHTC3_Read4BytesAndCrc(temp, humi);
	}

	return error;
}

//------------------------------------------------------------------------------
etError SHTC3_GetTempAndHumiPolling(float *temp, float *humi)
{
	etError error;			 // error code
	uint8_t maxPolling = 20; // max. retries to read the measurement (polling)
	uint16_t rawValueTemp;	 // temperature raw value from sensor
	uint16_t rawValueHumi;	 // humidity raw value from sensor

	// measure, read temperature first, clock streching disabled (polling)
	error = SHTC3_WriteCommand(MEAS_T_RH_POLLING);

	// if no error, ...
	if (error == NO_ERROR)
	{
		// poll every 1ms for measurement ready
		while (maxPolling--)
		{
			// check if the measurement has finished
			error = SHTC3_Read4BytesAndCrc(&rawValueTemp, &rawValueHumi);

			// if measurement has finished -> exit loop
			if (error == NO_ERROR)
				break;

			// delay 1ms
			_shtc3_dev.delay_us(1000);
		}
	}

	// if no error, calculate temperature in C and humidity in %RH
	if (error == NO_ERROR)
	{
		*temp = SHTC3_CalcTemperature(rawValueTemp);
		*humi = SHTC3_CalcHumidity(rawValueHumi);
	}

	return error;
}

//------------------------------------------------------------------------------
etError SHTC3_GetTempAndHumiPollingRaw(uint16_t *temp, uint16_t *humi)
{
	etError error;			 // error code
	uint8_t maxPolling = 20; // max. retries to read the measurement (polling)

	// measure, read temperature first, clock streching disabled (polling)
	error = SHTC3_WriteCommand(MEAS_T_RH_POLLING);

	// if no error, ...
	if (error == NO_ERROR)
	{
		// poll every 1ms for measurement ready
		while (maxPolling--)
		{
			// check if the measurement has finished
			error = SHTC3_Read4BytesAndCrc(temp, humi);

			// if measurement has finished -> exit loop
			if (error == NO_ERROR)
				break;

			// delay 1ms
			_shtc3_dev.delay_us(1000);
		}
	}

	return error;
}

//------------------------------------------------------------------------------
etError SHTC3_GetId(uint16_t *id)
{
	etError error; // error code

	// write ID read command
	error = SHTC3_WriteCommand(READ_ID);

	// if no error, read ID
	if (error == NO_ERROR)
	{
		error = SHTC3_Read2BytesAndCrc(id);
	}

	return error;
}

//------------------------------------------------------------------------------
etError SHTC3_Sleep(void)
{
	etError error = SHTC3_WriteCommand(SLEEP);

	return error;
}

//------------------------------------------------------------------------------
etError SHTC3_Wakeup(void)
{
	etError error = SHTC3_WriteCommand(WAKEUP);

	_shtc3_dev.delay_us(100); // wait 100 us

	return error;
}

//------------------------------------------------------------------------------
etError SHTC3_SoftReset(void)
{
	etError error; // error code

	// write reset command
	error = SHTC3_WriteCommand(SOFT_RESET);

	return error;
}

//------------------------------------------------------------------------------
static etError SHTC3_WriteCommand(etCommands cmd)
{
	etError error; // error code
	uint8_t wBuffer[2];

	wBuffer[0] = cmd >> 8;
	wBuffer[1] = cmd & 0xFF;

	error = _shtc3_dev.i2c_write(_shtc3_dev.dev_id, wBuffer, 2);
	if (error != NO_ERROR)
		return I2C_ERROR;

	return error;
}

//------------------------------------------------------------------------------
static etError SHTC3_Read2BytesAndCrc(uint16_t *data)
{
	etError error;		// error code
	uint8_t rBuffer[3]; // read data array
	uint8_t checksum;	// checksum byte

	// read two data bytes and one checksum byte
	error = _shtc3_dev.i2c_read(_shtc3_dev.dev_id, rBuffer, 3);
	if (error != NO_ERROR)
		return I2C_ERROR;

	checksum = rBuffer[2];

	// verify checksum
	error = SHTC3_CheckCrc(&rBuffer[0], 2, checksum);

	// combine the two bytes to a 16-bit value
	*data = (rBuffer[0] << 8) | rBuffer[1];

	return error;
}

//------------------------------------------------------------------------------
static etError SHTC3_Read4BytesAndCrc(uint16_t *data1, uint16_t *data2)
{
	etError error;		// error code
	uint8_t rBuffer[6]; // read data array
	uint8_t checksum;	// checksum byte

	// read two data bytes and one checksum byte
	error = _shtc3_dev.i2c_read(_shtc3_dev.dev_id, rBuffer, 6);
	if (error != NO_ERROR)
		return I2C_ERROR;

	checksum = rBuffer[2];

	// verify checksum
	error = SHTC3_CheckCrc(&rBuffer[0], 2, checksum);
	if (error != NO_ERROR)
		return error;

	checksum = rBuffer[5];

	// verify checksum
	error = SHTC3_CheckCrc(&rBuffer[3], 2, checksum);
	if (error != NO_ERROR)
		return error;

	// combine the two bytes to a 16-bit value
	*data1 = (rBuffer[0] << 8) | rBuffer[1];
	*data2 = (rBuffer[3] << 8) | rBuffer[4];

	return error;
}

//------------------------------------------------------------------------------
static etError SHTC3_CheckCrc(uint8_t data[], uint8_t nbrOfBytes,
							  uint8_t checksum)
{
	uint8_t crc = 0xFF; // calculated checksum

	// calculates 8-Bit checksum with given polynomial
	for (uint8_t byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++)
	{
		crc ^= (data[byteCtr]);
		for (uint8_t bit = 8; bit > 0; --bit)
		{
			if (crc & 0x80)
			{
				crc = (crc << 1) ^ CRC_POLYNOMIAL;
			}
			else
			{
				crc = (crc << 1);
			}
		}
	}

	// verify checksum
	if (crc != checksum)
	{
		return CHECKSUM_ERROR;
	}
	else
	{
		return NO_ERROR;
	}
}

//------------------------------------------------------------------------------
static float SHTC3_CalcTemperature(uint16_t rawValue)
{
	// calculate temperature [C]
	// T = -45 + 175 * rawValue / 2^16
	return 175 * (float)rawValue / 65536.0f - 45.0f;
}

//------------------------------------------------------------------------------
static float SHTC3_CalcHumidity(uint16_t rawValue)
{
	// calculate relative humidity [%RH]
	// RH = rawValue / 2^16 * 100
	return 100 * (float)rawValue / 65536.0f;
}