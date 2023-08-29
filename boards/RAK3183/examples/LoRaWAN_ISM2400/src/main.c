//*****************************************************************************
//
//! @file hello_world_uart.c
//!
//! @brief A simple "Hello World" example using the UART peripheral.
//!
//! This example prints a "Hello World" message with some device info
//! over UART at 115200 baud.
//! To see the output of this program, run a terminal appl such as
//! Tera Term or PuTTY, and configure the console for UART.
//! The example sleeps after it is done printing.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2021, Ambiq Micro, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision release_sdk_3_0_0-742e5ac27c of the AmbiqSuite Development Package.
//
//*****************************************************************************

#include "am_mcu_apollo.h"
#include "am_util.h"

#include "smtc_hal_gpio.h"
#include "smtc_hal_lp_timer.h"
#include "smtc_hal_mcu.h"
#include "i2c.h"

#define LED1 44
#define LED2 45

//*****************************************************************************
//
// UART handle.
//
//*****************************************************************************
char uart_input(void);
void *phUART;

#define CHECK_ERRORS(x)               \
    if ((x) != AM_HAL_STATUS_SUCCESS) \
    {                                 \
        error_handler(x);             \
    }
volatile uint32_t ui32LastError;

//*****************************************************************************
//
// Catch HAL errors.
//
//*****************************************************************************
void error_handler(uint32_t ui32ErrorStatus)
{
    ui32LastError = ui32ErrorStatus;

    while (1)
        ;
}

//*****************************************************************************
//
// UART buffers.
//
//*****************************************************************************
uint8_t g_pui8TxBuffer[1024];
uint8_t g_pui8RxBuffer[2];

//*****************************************************************************
//
// UART configuration.
//
//*****************************************************************************
const am_hal_uart_config_t g_sUartConfig =
    {
        //
        // Standard UART settings: 115200-8-N-1
        //
        .ui32BaudRate = 115200,
        .ui32DataBits = AM_HAL_UART_DATA_BITS_8,
        .ui32Parity = AM_HAL_UART_PARITY_NONE,
        .ui32StopBits = AM_HAL_UART_ONE_STOP_BIT,
        .ui32FlowControl = AM_HAL_UART_FLOW_CTRL_NONE,

        //
        // Set TX and RX FIFOs to interrupt at half-full.
        //
        .ui32FifoLevels = (AM_HAL_UART_TX_FIFO_7_8 |
                           AM_HAL_UART_TX_FIFO_7_8),

        //
        // Buffers
        //
        .pui8TxBuffer = g_pui8TxBuffer,
        .ui32TxBufferSize = sizeof(g_pui8TxBuffer),
        .pui8RxBuffer = g_pui8RxBuffer,
        .ui32RxBufferSize = sizeof(g_pui8RxBuffer),
};

//*****************************************************************************
//
// UART0 interrupt handler.
//
//*****************************************************************************
void am_uart_isr(void)
{
    //
    // Service the FIFOs as necessary, and clear the interrupts.
    //
    uint32_t ui32Status, ui32Idle;
    am_hal_uart_interrupt_status_get(phUART, &ui32Status, true);
    am_hal_uart_interrupt_clear(phUART, ui32Status);
    am_hal_uart_interrupt_service(phUART, ui32Status, &ui32Idle);

    //am_hal_gpio_state_write(LED1, AM_HAL_GPIO_OUTPUT_TOGGLE);
}

//*****************************************************************************
//
// UART print string0
//
//*****************************************************************************
void uart_print(char *pcStr)
{
    uint32_t ui32StrLen = 0;
    uint32_t ui32BytesWritten = 0;

    //
    // Measure the length of the string.
    //
    while (pcStr[ui32StrLen] != 0)
    {
        ui32StrLen++;
    }

    //
    // Print the string via the UART.
    //
    const am_hal_uart_transfer_t sUartWrite =
        {
            .ui32Direction = AM_HAL_UART_WRITE,
            .pui8Data = (uint8_t *)pcStr,
            .ui32NumBytes = ui32StrLen,
            .ui32TimeoutMs = AM_HAL_UART_WAIT_FOREVER,
            .pui32BytesTransferred = &ui32BytesWritten,
        };

    CHECK_ERRORS(am_hal_uart_transfer(phUART, &sUartWrite));

//    if (ui32BytesWritten != ui32StrLen)
//    {
//        //
//        // Couldn't send the whole string!!
//        //
//        while (1)
//            ;
//    }
}

void init_rak3183_led()
{
    hal_gpio_init_out(LED1, 0);
    hal_gpio_init_out(LED2, 0);
}

//*****************************************************************************
//
// Main
//
//*****************************************************************************
int main(void)
{
    //
    // Set the clock frequency.
    //
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

    //
    // Set the default cache configuration
    //
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    //
    // Configure the board for low power operation.
    //
    // am_bsp_low_power_init();

    //
    // Initialize the printf interface for UART output.
    //
    CHECK_ERRORS(am_hal_uart_initialize(0, &phUART));

    CHECK_ERRORS(am_hal_uart_power_control(phUART, AM_HAL_SYSCTRL_WAKE, false));

    am_hal_uart_clock_speed_e eUartClockSpeed = eUART_CLK_SPEED_DEFAULT;
    CHECK_ERRORS(am_hal_uart_control(phUART, AM_HAL_UART_CONTROL_CLKSEL, &eUartClockSpeed));
    CHECK_ERRORS(am_hal_uart_configure(phUART, &g_sUartConfig));

    //
    // Enable the UART pins.
    //
    const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_COM_UART_TX0 =
        {
            .uFuncSel = AM_HAL_PIN_39_UART0TX,
        };

    const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_COM_UART_RX0 =
        {
            .uFuncSel = AM_HAL_PIN_40_UART0RX};
    //
    am_hal_gpio_pinconfig(39, g_AM_BSP_GPIO_COM_UART_TX0);
    am_hal_gpio_pinconfig(40, g_AM_BSP_GPIO_COM_UART_RX0);

    //
    // Enable interrupts.
    //
    NVIC_SetPriority(UART0_IRQn, 7);
    NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn));
    am_hal_interrupt_master_enable();

    //
    // Set the main print interface to use the UART print function we defined.
    //
    am_util_stdio_printf_init(uart_print);

    //
    // Print the banner.
    //
    am_util_stdio_terminal_clear();

    init_rak3183_led();
		am_util_stdio_printf("Version: 1.0.2 LPP\n");
    lorawan_init();
		
		i2c_init();

    char character = 0;

    while (1)
    {
        smtc_modem_run_engine();

        character = uart_input();
        if (character)
        {
            am_hal_gpio_state_write(LED2, AM_HAL_GPIO_OUTPUT_TOGGLE);
            process_serial_input(character);
        }
        //
        // Go to Deep Sleep.
        //
        // am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    }
}

char uart_input(void)
{
    static char pcStr;
    uint32_t ui32BytesRread = 0;
    const am_hal_uart_transfer_t sUartRead =
        {
            .ui32Direction = AM_HAL_UART_READ,
            .pui8Data = (uint8_t *)&pcStr,
            .ui32NumBytes = 1,
            .ui32TimeoutMs = 0,
            .pui32BytesTransferred = &ui32BytesRread,
        };

    CHECK_ERRORS(am_hal_uart_transfer(phUART, &sUartRead));
    if (ui32BytesRread == 1)
    {
        // echo test
        const am_hal_uart_transfer_t sUartWrite =
            {
                .ui32Direction = AM_HAL_UART_WRITE,
                .pui8Data = (uint8_t *)&pcStr,
                .ui32NumBytes = 1,
                .ui32TimeoutMs = 0,
                .pui32BytesTransferred = &ui32BytesRread,
            };

        CHECK_ERRORS(am_hal_uart_transfer(phUART, &sUartWrite));

        return pcStr;
    }
    else
    {
        return 0;
    }
}
