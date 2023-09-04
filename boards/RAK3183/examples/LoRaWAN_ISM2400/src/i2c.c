#include "i2c.h"

#define IOM2_MODULE 2

#define I2C_SDA 25
#define I2C_SCL 27

// LIS3DH�Ĵ�������

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

int16_t val[3];

void RAK1904(void);

uint8_t lis3dh_initialized = false;

void *iom2_phandle;

static am_hal_iom_config_t g_sIOMI2cConfig =
	{
		.eInterfaceMode = AM_HAL_IOM_I2C_MODE,
		.ui32ClockFreq = AM_HAL_IOM_100KHZ,
};

const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_IOM2_SDA =
	{
		.uFuncSel = AM_HAL_PIN_25_M2SDAWIR3,
		.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K,
		.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
		.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
		.uIOMnum = 2};

const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_IOM2_SCL =
	{
		.uFuncSel = AM_HAL_PIN_27_M2SCL,
		.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K,
		.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
		.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
		.uIOMnum = 2};

void i2c_init()
{

	if (am_hal_iom_initialize(IOM2_MODULE, &iom2_phandle) ||
		am_hal_iom_power_ctrl(iom2_phandle, AM_HAL_SYSCTRL_WAKE, false) ||
		am_hal_iom_configure(iom2_phandle, (am_hal_iom_config_t *)&g_sIOMI2cConfig) ||
		am_hal_iom_enable(iom2_phandle))
	{
		am_util_stdio_printf("ERROR am_hal_iom_initialize\r\n");
		while (1)
			;
	}

	am_hal_gpio_pinconfig(I2C_SDA, g_AM_BSP_GPIO_IOM2_SDA);
	am_hal_gpio_pinconfig(I2C_SCL, g_AM_BSP_GPIO_IOM2_SCL);

	am_util_stdio_printf("IIC initialize.\n");
	RAK1904();
}

void RAK1904(void)
{

	uint16_t id;
	uint32_t pBuf[10] = {0};
	uint32_t offset = 0x0f;
	iom_slave_read_offset(offset, pBuf, 1);

	if(pBuf[0]==0x33)
	{
		am_util_stdio_printf("RAK1904 ID 0x%02X\n", pBuf[0]);
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
	iom_slave_read_offset(offset, pBuf, 1);
	if (0x08 & (uint8_t)pBuf[0])
	{
		lis3dh_acceleration_raw_get();
	}
}

void iom_slave_read_offset(uint32_t offset, uint32_t *pBuf, uint32_t size)
{
	int ret;
	am_hal_iom_transfer_t Transaction;
	//
	//! Instruction length (0,1,2, or 3).
	//
	Transaction.ui32InstrLen = 1;
	Transaction.ui32Instr = offset;
	Transaction.eDirection = AM_HAL_IOM_RX;
	Transaction.ui32NumBytes = size;
	Transaction.pui32RxBuffer = pBuf;
	Transaction.bContinue = false;
	Transaction.ui8RepeatCount = 0;
	Transaction.ui32PauseCondition = 0;
	Transaction.ui32StatusSetClr = 0;

	Transaction.uPeerInfo.ui32I2CDevAddr = 0x18; /* LIS3DH */

	ret = am_hal_iom_blocking_transfer(iom2_phandle, &Transaction);
	if (ret != 0)
	{
		am_util_stdio_printf("am_hal_iom_blocking_transfer error\n");
	}
}

void iom_slave_write_offset(uint32_t offset, uint32_t *pBuf, uint32_t size)
{
	int ret;
	am_hal_iom_transfer_t Transaction;

	Transaction.ui32InstrLen = 1;
	Transaction.ui32Instr = offset;
	Transaction.eDirection = AM_HAL_IOM_TX;
	Transaction.ui32NumBytes = size;
	Transaction.pui32TxBuffer = pBuf;
	Transaction.bContinue = false;
	Transaction.ui8RepeatCount = 0;
	Transaction.ui32PauseCondition = 0;
	Transaction.ui32StatusSetClr = 0;

	Transaction.uPeerInfo.ui32I2CDevAddr = 0x18;

	ret = am_hal_iom_blocking_transfer(iom2_phandle, &Transaction);
	if (ret != 0)
	{
		am_util_stdio_printf("am_hal_iom_blocking_transfer error\n");
	}
}

void iom_slave_read(uint32_t *pBuf, uint32_t size)
{
	int ret;
	am_hal_iom_transfer_t Transaction;

	Transaction.ui32InstrLen = 0;
	Transaction.ui32Instr = 0;
	Transaction.eDirection = AM_HAL_IOM_RX;
	Transaction.ui32NumBytes = size;
	Transaction.pui32RxBuffer = pBuf;
	Transaction.bContinue = false;
	Transaction.ui8RepeatCount = 0;
	Transaction.ui32PauseCondition = 0;
	Transaction.ui32StatusSetClr = 0;

	Transaction.uPeerInfo.ui32I2CDevAddr = 0x18;

	ret = am_hal_iom_blocking_transfer(iom2_phandle, &Transaction);
	if (ret != 0)
	{
		am_util_stdio_printf("am_hal_iom_blocking_transfer error\n");
	}
}

void iom_slave_write(uint32_t *pBuf, uint32_t size)
{
	int ret;
	am_hal_iom_transfer_t Transaction;

	Transaction.ui32InstrLen = 0;
	Transaction.ui32Instr = 0;
	Transaction.eDirection = AM_HAL_IOM_TX;
	Transaction.ui32NumBytes = size;
	Transaction.pui32TxBuffer = pBuf;
	Transaction.bContinue = false;
	Transaction.ui8RepeatCount = 0;
	Transaction.ui32PauseCondition = 0;
	Transaction.ui32StatusSetClr = 0;

	Transaction.uPeerInfo.ui32I2CDevAddr = 0x18;

	ret = am_hal_iom_blocking_transfer(iom2_phandle, &Transaction);
	if (ret != 0)
	{
		am_util_stdio_printf("am_hal_iom_blocking_transfer error\n");
	}
}

int32_t lis3dh_block_data_update_set(void)
{
	int ret = 0;
	uint16_t id;
	uint32_t cmd;
	uint32_t offset = LIS3DH_CTRL_REG4;
	iom_slave_read_offset(offset, &cmd, 1);
	// am_util_stdio_printf("lis3dh_block_data_update_set\n");
	// am_util_stdio_printf("LIS3DH_CTRL_REG4 %02X\n",cmd);
	// cmd = cmd &  ~0x80 ;                           //no block
	cmd = cmd | 0x80;
	iom_slave_write_offset(offset, &cmd, 1);

	iom_slave_read_offset(offset, &cmd, 1);
	// am_util_stdio_printf("RESULT LIS3DH_CTRL_REG4 %02X\n",cmd);

	return ret;
}

int32_t lis3dh_data_rate_set(void)
{
	// am_util_stdio_printf("lis3dh_data_rate_set\n");
	int ret = 0;
	uint16_t id;
	uint32_t cmd;
	uint32_t offset = LIS3DH_CTRL_REG1;
	iom_slave_read_offset(offset, &cmd, 1); // HR / Normal / Low-power mode (1 Hz)
	// am_util_stdio_printf("LIS3DH_CTRL_REG1 %02X\n",cmd);

	cmd &= 0X0F;
	cmd = cmd | 0x10;
	iom_slave_write_offset(offset, &cmd, 1);

	iom_slave_read_offset(offset, &cmd, 1);
	// am_util_stdio_printf("RESULT LIS3DH_CTRL_REG1 %02X\n",cmd);
	return ret;
}

int32_t lis3dh_operating_mode_set(void)
{
	/* Set device in continuous mode with 12 bit resol. */
	int ret = 0;
	uint16_t id;
	uint32_t cmd;
	uint32_t offset = LIS3DH_CTRL_REG1;
	iom_slave_read_offset(offset, &cmd, 1);
	cmd &= ~0x08;
	iom_slave_write_offset(offset, &cmd, 1);

	offset = LIS3DH_CTRL_REG4;
	iom_slave_read_offset(offset, &cmd, 1);
	cmd |= 0x08;
	iom_slave_write_offset(offset, &cmd, 1);

	return ret;
}

int32_t lis3dh_full_scale_set(void)
{
	// am_util_stdio_printf("lis3dh_full_scale_set\n");
	int ret = 0;
	uint16_t id;
	uint32_t cmd;
	uint32_t offset = LIS3DH_CTRL_REG4;
	iom_slave_read_offset(offset, &cmd, 1); // HR / Normal / Low-power mode (1 Hz)
	// am_util_stdio_printf("LIS3DH_CTRL_REG4 %02X\n",cmd);

	cmd &= ~0X30;
	iom_slave_write_offset(offset, &cmd, 1);

	iom_slave_read_offset(offset, &cmd, 1);
	// am_util_stdio_printf("RESULT LIS3DH_CTRL_REG4 %02X\n",cmd);
	return ret;
}

int32_t lis3dh_acceleration_raw_get()
{
	uint32_t offset = LIS3DH_OUT_X_L;
	uint32_t pbuf[2];
	iom_slave_read_offset(offset, pbuf, 1);

	offset = LIS3DH_OUT_X_H;
	iom_slave_read_offset(offset, &pbuf[1], 1);
	val[0] = ((pbuf[1] << 8) | (pbuf[0]));
	// am_util_stdio_printf("X %d\n",val[0]);
	val[0] = val[0] / 16;

	offset = LIS3DH_OUT_Y_L;
	iom_slave_read_offset(offset, pbuf, 1);

	offset = LIS3DH_OUT_Y_H;
	iom_slave_read_offset(offset, &pbuf[1], 1);
	val[1] = ((pbuf[1] << 8) | (pbuf[0]));
	// am_util_stdio_printf("Y %d\n",val[1]);
	val[1] = val[1] / 16;

	offset = LIS3DH_OUT_Z_L;
	iom_slave_read_offset(offset, pbuf, 1);

	offset = LIS3DH_OUT_Z_H;
	iom_slave_read_offset(offset, &pbuf[1], 1);
	val[2] = ((pbuf[1] << 8) | (pbuf[0]));
	// am_util_stdio_printf("Z %d\n",val[2]);
	val[2] = val[2] / 16;

	am_util_stdio_printf("  ******************************************************************\n");
	am_util_stdio_printf("  * Lis3dh acceleration [x %d mg \t y %d mg \t z %d mg]\n", val[0], val[1], val[2]);
	am_util_stdio_printf("  ******************************************************************\n");
}
