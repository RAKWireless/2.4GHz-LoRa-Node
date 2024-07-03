#include "i2c.h"
#include "rak1904.h"

#define IOM2_MODULE 2

#define I2C_SDA 25
#define I2C_SCL 27

void *iom2_phandle;

static am_hal_iom_config_t g_sIOMI2cConfig =
	{
		.eInterfaceMode = AM_HAL_IOM_I2C_MODE,
		.ui32ClockFreq = AM_HAL_IOM_400KHZ,
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

	//am_util_stdio_printf("IIC initialize.\r\n");
	// Try to initialize RAK1904
	RAK1904();
	// Try to initialize RAK1901
	RAK1901();
}

bool iom_slave_read_offset(uint32_t offset, uint32_t *pBuf, uint32_t size, uint8_t i2c_addr)
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

	Transaction.uPeerInfo.ui32I2CDevAddr = i2c_addr; /* LIS3DH */

	ret = am_hal_iom_blocking_transfer(iom2_phandle, &Transaction);
	if (ret != 0)
	{
		//am_util_stdio_printf("am_hal_iom_blocking_transfer error\r\n");
		return false;
	}
	return true;
}

bool iom_slave_write_offset(uint32_t offset, uint32_t *pBuf, uint32_t size, uint8_t i2c_addr)
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

	Transaction.uPeerInfo.ui32I2CDevAddr = i2c_addr;

	ret = am_hal_iom_blocking_transfer(iom2_phandle, &Transaction);
	if (ret != 0)
	{
		am_util_stdio_printf("am_hal_iom_blocking_transfer error\r\n");
		return false;
	}
	return true;
}

bool iom_slave_read(uint32_t *pBuf, uint32_t size, uint8_t i2c_addr)
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

	Transaction.uPeerInfo.ui32I2CDevAddr = i2c_addr;

	ret = am_hal_iom_blocking_transfer(iom2_phandle, &Transaction);
	if (ret != 0)
	{
		am_util_stdio_printf("am_hal_iom_blocking_transfer error\r\n");
		return false;
	}
	return true;
}

bool iom_slave_write(uint32_t *pBuf, uint32_t size, uint8_t i2c_addr)
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

	Transaction.uPeerInfo.ui32I2CDevAddr = i2c_addr;

	ret = am_hal_iom_blocking_transfer(iom2_phandle, &Transaction);
	if (ret != 0)
	{
		//am_util_stdio_printf("am_hal_iom_blocking_transfer error\r\n");
		return false;
	}
	return true;
}
