/*!
 * \file      smtc_hal_spi.c
 *
 * \brief     SPI Hardware Abstraction Layer implementation
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>	 // C99 types
#include <stdbool.h> // bool type

/*===============================modify=======================================*/
#include "smtc_hal_spi.h"
#include "am_mcu_apollo.h"
#include "am_util.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

#define IOM1_MODULE 1
#define AM_SX1280_GPIO_IOM0_SCK 8
#define AM_SX1280_GPIO_IOM0_MISO 9
#define AM_SX1280_GPIO_IOM0_MOSI 10

void *iom1_pphandle;

am_hal_iom_config_t IOM0Config =
	{
		.eInterfaceMode = AM_HAL_IOM_SPI_MODE,
		.ui32ClockFreq = AM_HAL_IOM_2MHZ,
		.eSpiMode = AM_HAL_IOM_SPI_MODE_0,
		.pNBTxnBuf = 0,
		.ui32NBTxnBufLength = 0};

void hal_spi_init(const uint32_t id, const hal_gpio_pin_names_t mosi, const hal_gpio_pin_names_t miso,
				  const hal_gpio_pin_names_t sclk)
{

	if (am_hal_iom_initialize(IOM1_MODULE, &iom1_pphandle) ||
		am_hal_iom_power_ctrl(iom1_pphandle, AM_HAL_SYSCTRL_WAKE, false) ||
		am_hal_iom_configure(iom1_pphandle, (am_hal_iom_config_t *)&IOM0Config) ||
		am_hal_iom_enable(iom1_pphandle))
	{
		am_util_stdio_printf("ERROR am_hal_iom_initialize\r\n");
		while (1)
			;
	}

	const am_hal_gpio_pincfg_t AM_BSP_GPIO_IOM0_SCK_pincfg =
		{
			.uFuncSel = AM_HAL_PIN_8_M1SCK,
			.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
			.uIOMnum = 1};

	const am_hal_gpio_pincfg_t AM_BSP_GPIO_IOM0_MISO_pincfg =
		{
			.uFuncSel = AM_HAL_PIN_9_M1MISO,
			.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
			.uIOMnum = 1};

	const am_hal_gpio_pincfg_t AM_BSP_GPIO_IOM0_MOSI_pincfg =
		{
			.uFuncSel = AM_HAL_PIN_10_M1MOSI,
			.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
			.uIOMnum = 1};

	am_hal_gpio_pinconfig(AM_SX1280_GPIO_IOM0_SCK, AM_BSP_GPIO_IOM0_SCK_pincfg);
	am_hal_gpio_pinconfig(AM_SX1280_GPIO_IOM0_MISO, AM_BSP_GPIO_IOM0_MISO_pincfg);
	am_hal_gpio_pinconfig(AM_SX1280_GPIO_IOM0_MOSI, AM_BSP_GPIO_IOM0_MOSI_pincfg);
}

void hal_spi_de_init(const uint32_t id)
{
}

uint16_t hal_spi_in_out(const uint32_t id, const uint16_t out_data)
{
	hal_mcu_disable_irq();
	uint8_t rxData = 0;

	uint32_t TxBuffer;
	uint32_t RxBuffer;

	am_hal_iom_transfer_t Transaction =
		{
			.uPeerInfo.ui32SpiChipSelect = 0,
			.ui32InstrLen = 0,
			.ui32Instr = 0,
			.ui32NumBytes = 1,
			.eDirection = AM_HAL_IOM_FULLDUPLEX,
			.pui32TxBuffer = &TxBuffer,
			.pui32RxBuffer = &RxBuffer,
			.bContinue = false,
			.ui8RepeatCount = 0,
			.ui8Priority = 0};

	TxBuffer = out_data;

	if (am_hal_iom_spi_blocking_fullduplex(iom1_pphandle, &Transaction) != 0)
	{
		am_util_stdio_printf("spi fullduplex error!\r\n");
	}
	rxData = (uint8_t)RxBuffer;
	hal_mcu_enable_irq();
	return (rxData);
}


void hal_spi_sleep()
{
	am_hal_iom_disable(iom1_pphandle);
	am_hal_iom_power_ctrl(iom1_pphandle,AM_HAL_SYSCTRL_DEEPSLEEP,false);
}
/* --- EOF ------------------------------------------------------------------ */
