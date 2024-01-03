/**
 * @file rak1904.c
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Init and read functions for RAK1904
 * @version 0.1
 * @date 2023-09-19
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "rak1904.h"

int16_t val[10];
uint8_t lis3dh_initialized = false;

void RAK1904(void)
{

	uint16_t id;
	uint32_t pBuf[10] = {0};
	uint32_t offset = 0x0f;
	if (!iom_slave_read_offset(offset, pBuf, 1, LIS3DH_I2C_ADDR))
	{
		//am_util_stdio_printf("RAK1904 read failed\r\n");
		return;
	}

	if (pBuf[0] == 0x33)
	{
		am_util_stdio_printf("RAK1904 ID 0x%02X\r\n", pBuf[0]);
		lis3dh_initialized = true;
	}
	else
	{
		return;
	}

	lis3dh_block_data_update_set();
	lis3dh_data_rate_set();
	lis3dh_operating_mode_set();
	lis3dh_full_scale_set();
	return;
}

void RAK1904_func(void)
{
	uint32_t pBuf[10] = {0};
	uint32_t offset = 0x0f;
	offset = LIS3DH_STATUS_REG;
	if (iom_slave_read_offset(offset, pBuf, 1, LIS3DH_I2C_ADDR))
	{
		if (0x08 & (uint8_t)pBuf[0])
		{
			lis3dh_acceleration_raw_get();
		}
	}
}

int32_t lis3dh_block_data_update_set(void)
{
	int ret = 0;
	uint16_t id;
	uint32_t cmd;
	uint32_t offset = LIS3DH_CTRL_REG4;
	if (!iom_slave_read_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR))
	{
		// am_util_stdio_printf("lis3dh_block_data_update_set\r\n");
		// am_util_stdio_printf("LIS3DH_CTRL_REG4 %02X\r\n",cmd);
		// cmd = cmd &  ~0x80 ;                           //no block
		cmd = cmd | 0x80;
		if (iom_slave_write_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR))
		{
			iom_slave_read_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR);
			// am_util_stdio_printf("RESULT LIS3DH_CTRL_REG4 %02X\r\n",cmd);
		}
	}
	return ret;
}

int32_t lis3dh_data_rate_set(void)
{
	// am_util_stdio_printf("lis3dh_data_rate_set\r\n");
	int ret = 0;
	uint16_t id;
	uint32_t cmd;
	uint32_t offset = LIS3DH_CTRL_REG1;
	if (iom_slave_read_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR)) // HR / Normal / Low-power mode (1 Hz)
	{
		// am_util_stdio_printf("LIS3DH_CTRL_REG1 %02X\r\n",cmd);

		cmd &= 0X0F;
		cmd = cmd | 0x10;
		if (iom_slave_write_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR))
		{
			iom_slave_read_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR);
			// am_util_stdio_printf("RESULT LIS3DH_CTRL_REG1 %02X\r\n",cmd);
		}
	}
	return ret;
}

int32_t lis3dh_operating_mode_set(void)
{
	/* Set device in continuous mode with 12 bit resol. */
	int ret = 0;
	uint16_t id;
	uint32_t cmd;
	uint32_t offset = LIS3DH_CTRL_REG1;
	if (iom_slave_read_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR))
	{
		cmd &= ~0x08;
		if (iom_slave_write_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR))
		{
			offset = LIS3DH_CTRL_REG4;
			if (iom_slave_read_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR))
			{
				cmd |= 0x08;
				iom_slave_write_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR);
			}
		}
	}

	return ret;
}

int32_t lis3dh_full_scale_set(void)
{
	// am_util_stdio_printf("lis3dh_full_scale_set\r\n");
	int ret = 0;
	uint16_t id;
	uint32_t cmd;
	uint32_t offset = LIS3DH_CTRL_REG4;
	if (iom_slave_read_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR)) // HR / Normal / Low-power mode (1 Hz)
	{															 // am_util_stdio_printf("LIS3DH_CTRL_REG4 %02X\r\n",cmd);

		cmd &= ~0X30;
		if (iom_slave_write_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR))
		{
			iom_slave_read_offset(offset, &cmd, 1, LIS3DH_I2C_ADDR);
			// am_util_stdio_printf("RESULT LIS3DH_CTRL_REG4 %02X\r\n",cmd);
		}
	}
	return ret;
}

int32_t lis3dh_acceleration_raw_get()
{
	uint32_t offset = LIS3DH_OUT_X_L;
	uint32_t pbuf[2];
	if (iom_slave_read_offset(offset, pbuf, 1, LIS3DH_I2C_ADDR))
	{
		offset = LIS3DH_OUT_X_H;
		if (iom_slave_read_offset(offset, &pbuf[1], 1, LIS3DH_I2C_ADDR))
		{
			val[0] = ((pbuf[1] << 8) | (pbuf[0]));
			// am_util_stdio_printf("X %d\r\n",val[0]);
			val[0] = val[0] / 16;

			offset = LIS3DH_OUT_Y_L;
			if (iom_slave_read_offset(offset, pbuf, 1, LIS3DH_I2C_ADDR))
			{
				offset = LIS3DH_OUT_Y_H;
				if (iom_slave_read_offset(offset, &pbuf[1], 1, LIS3DH_I2C_ADDR))
				{
					val[1] = ((pbuf[1] << 8) | (pbuf[0]));
					// am_util_stdio_printf("Y %d\r\n",val[1]);
					val[1] = val[1] / 16;

					offset = LIS3DH_OUT_Z_L;
					if (iom_slave_read_offset(offset, pbuf, 1, LIS3DH_I2C_ADDR))
					{
						offset = LIS3DH_OUT_Z_H;
						if (iom_slave_read_offset(offset, &pbuf[1], 1, LIS3DH_I2C_ADDR))
						{
							val[2] = ((pbuf[1] << 8) | (pbuf[0]));
							// am_util_stdio_printf("Z %d\r\n",val[2]);
							val[2] = val[2] / 16;

							am_util_stdio_printf("  ******************************************************************\r\n");
							am_util_stdio_printf("  * Lis3dh acceleration [x %d mg \t y %d mg \t z %d mg]\r\n", val[0], val[1], val[2]);
							am_util_stdio_printf("  ******************************************************************\r\n");
						}
					}
				}
			}
		}
	}
	else
	{
		am_util_stdio_printf("  Error reading Lis3dh\r\n");
	}
}
