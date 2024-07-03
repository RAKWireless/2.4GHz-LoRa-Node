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
static uint32_t uart_input(uint8_t *buf, uint32_t *len);

void *phUART;

//*****************************************************************************
//
// Catch HAL errors.
//
//*****************************************************************************
#define CHECK_ERRORS(x)               \
    if ((x) != AM_HAL_STATUS_SUCCESS) \
    {                                 \
        error_handler(x);             \
    }
volatile uint32_t ui32LastError;

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
#define UART_FIFO_MAX 32
#define RING_BUFFER_SIZE 512

typedef struct
{
    char buffer[RING_BUFFER_SIZE];
    int head;
    int tail;
} RingBuffer;

uint8_t g_pui8TxBuffer[512];
uint8_t g_pui8RxBuffer[UART_FIFO_MAX]; // FIFO MIX 32 Byte
static RingBuffer rxRingBuffer = {{0}, 0, 0};

// 单字符存储函数
void ringBufferPutChar(RingBuffer *buffer, uint8_t c)
{
    int next = (buffer->head + 1) % RING_BUFFER_SIZE;
    if (next != buffer->tail)
    { // Check buffer overflow
        buffer->buffer[buffer->head] = c;
        buffer->head = next;
    }
}

// 多字符存储函数
void ringBufferPutChars(RingBuffer *buffer, const uint8_t *c, int length)
{
    for (int i = 0; i < length; i++)
    {
        int next = (buffer->head + 1) % RING_BUFFER_SIZE;
        if (next != buffer->tail)
        { // Check buffer overflow
            buffer->buffer[buffer->head] = c[i];
            buffer->head = next;
        }
        else
        {
            // 如果没有足够的空间，可以选择停止或者处理错误
            break;
        }
    }
}

bool ringBufferGetChar(RingBuffer *buffer, uint8_t *c)
{
    if (buffer->tail == buffer->head)
    {
        return false; // Buffer is empty
    }
    *c = buffer->buffer[buffer->tail];
    buffer->tail = (buffer->tail + 1) % RING_BUFFER_SIZE;
    return true;
}
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
                           AM_HAL_UART_RX_FIFO_7_8),

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
    uint32_t ui32Status, ui32Idle, receivedByte;
    uint8_t rxBuffer[UART_FIFO_MAX];

    am_hal_uart_interrupt_status_get(phUART, &ui32Status, true);
    am_hal_uart_interrupt_clear(phUART, ui32Status);
    am_hal_uart_interrupt_service(phUART, ui32Status, &ui32Idle);

    if (ui32Status & (AM_HAL_UART_INT_RX_TMOUT | AM_HAL_UART_INT_RX)) // 这里这个超时中断类似STM32空闲中断
    {
        uart_input(rxBuffer, &receivedByte); // 测试这个函数单次只能拿28个字节   可能是设置了FIFO的原因  寄存器最大FIFO是32个字节
        // am_util_stdio_printf("[%d]", receivedByte);
        rxBuffer[receivedByte] = 0;
        // am_util_stdio_printf("[%s]\r\n", rxBuffer);

        ringBufferPutChars(&rxRingBuffer, rxBuffer, receivedByte);
    }
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

    if (ui32BytesWritten != ui32StrLen)
    {
        //
        // Couldn't send the whole string!!
        //
        while (1)
            ;
    }
}

void init_rak3183_led()
{
    hal_gpio_init_out(LED1, 1);
    hal_gpio_init_out(LED2, 1);
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

    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);

    //
    // Wait for 1 second for the 32KHz XTAL to startup and stabilize.
    //
    am_util_delay_ms(100);

    //
    // Enable HFADJ.
    //
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_HFADJ_ENABLE, 0);

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
    //am_util_stdio_terminal_clear();

    init_rak3183_led();
    lorawan_init();
    i2c_init();

    uint8_t character = 0;

    am_util_stdio_printf("\r\nRAKwireless RAK3183\r\n");

    while (1)
    {
        smtc_modem_run_engine();

        while (ringBufferGetChar(&rxRingBuffer, &character))
        {
            // echo
            am_util_stdio_printf("%c", character);
            process_serial_input(character);
        }

        // character = uart_input();
        // if (character)
        // {
        //     am_hal_gpio_state_write(LED2, AM_HAL_GPIO_OUTPUT_TOGGLE);
        //     process_serial_input(character);
        // }
        //
        // Go to Deep Sleep.
        //
        // am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    }
}

uint32_t uart_input(uint8_t *buf, uint32_t *len)
{
    const am_hal_uart_transfer_t sUartRead =
        {
            .ui32Direction = AM_HAL_UART_READ,
            .pui8Data = buf,
            .ui32NumBytes = 32, // FIFO 最大一次32字节读取
            .ui32TimeoutMs = 0,
            .pui32BytesTransferred = len,
        };

    am_hal_uart_transfer(phUART, &sUartRead);

    return *(sUartRead.pui32BytesTransferred);
}
