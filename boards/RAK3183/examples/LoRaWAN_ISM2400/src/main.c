/**
 * @file main.c
 * @brief RAK3183 LoRaWAN 2.4GHz Node Main Application
 * @version 1.1.0
 * @date 2025-07-02
 * 
 * This file contains the main application for RAK3183 LoRaWAN node with UART
 * communication interface. The application provides AT command interface for
 * LoRaWAN configuration and operation.
 * 
 * Features:
 * - UART console with AT command interface
 * - LoRaWAN communication stack
 * - Ring buffer for UART data handling
 * - LED status indication
 * - I2C sensor interface
 * 
 * Hardware:
 * - UART0: Pin 39 (TX), Pin 40 (RX) at 115200 baud
 * - LED1: Pin 44, LED2: Pin 45
 * - Apollo3 Blue MCU
 */

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

/* ================================================================================================
 * INCLUDES
 * ================================================================================================ */

#include "am_mcu_apollo.h"
#include "am_util.h"
#include "at_cmd.h"

#include "smtc_hal_gpio.h"
#include "smtc_hal_lp_timer.h"
#include "smtc_hal_mcu.h"
#include "i2c.h"

/* ================================================================================================
 * CONSTANTS AND DEFINITIONS
 * ================================================================================================ */

#define LED1 44                     ///< LED1 GPIO pin number
#define LED2 45                     ///< LED2 GPIO pin number

#define UART_FIFO_MAX 32           ///< Maximum UART FIFO size
#define RING_BUFFER_SIZE 512       ///< Ring buffer size for UART data

/**
 * @brief Error checking macro for HAL functions
 */
#define CHECK_ERRORS(x)               \
    if ((x) != AM_HAL_STATUS_SUCCESS) \
    {                                 \
        error_handler(x);             \
    }

/* ================================================================================================
 * TYPE DEFINITIONS
 * ================================================================================================ */

/**
 * @brief Ring buffer structure for UART data storage
 */
typedef struct
{
    char buffer[RING_BUFFER_SIZE];  ///< Data buffer
    int head;                       ///< Write pointer
    int tail;                       ///< Read pointer
} RingBuffer;

/* ================================================================================================
 * GLOBAL VARIABLES
 * ================================================================================================ */

void *phUART;                                           ///< UART handle
volatile uint32_t ui32LastError;                       ///< Last error status

uint8_t g_pui8TxBuffer[512];                           ///< UART TX buffer
uint8_t g_pui8RxBuffer[UART_FIFO_MAX];                 ///< UART RX buffer
static RingBuffer rxRingBuffer = {{0}, 0, 0};          ///< RX ring buffer

/* ================================================================================================
 * UART CONFIGURATION
 * ================================================================================================ */

/**
 * @brief UART configuration structure
 * Standard settings: 115200-8-N-1 with FIFO levels
 */
const am_hal_uart_config_t g_sUartConfig =
{
    .ui32BaudRate = 115200,                             ///< Baud rate
    .ui32DataBits = AM_HAL_UART_DATA_BITS_8,           ///< Data bits
    .ui32Parity = AM_HAL_UART_PARITY_NONE,             ///< Parity
    .ui32StopBits = AM_HAL_UART_ONE_STOP_BIT,          ///< Stop bits
    .ui32FlowControl = AM_HAL_UART_FLOW_CTRL_NONE,     ///< Flow control
    .ui32FifoLevels = (AM_HAL_UART_TX_FIFO_7_8 |       ///< FIFO levels
                       AM_HAL_UART_RX_FIFO_7_8),
    .pui8TxBuffer = g_pui8TxBuffer,                     ///< TX buffer
    .ui32TxBufferSize = sizeof(g_pui8TxBuffer),         ///< TX buffer size
    .pui8RxBuffer = g_pui8RxBuffer,                     ///< RX buffer
    .ui32RxBufferSize = sizeof(g_pui8RxBuffer),         ///< RX buffer size
};

/* ================================================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ================================================================================================ */

static uint32_t uart_input(uint8_t *buf, uint32_t *len);
static void init_rak3183_led(void);
static void error_handler(uint32_t ui32ErrorStatus);

void uart_print(char *pcStr);
/* ================================================================================================
 * RING BUFFER MANAGEMENT
 * ================================================================================================ */

/**
 * @brief Put a single character into ring buffer
 * @param buffer Pointer to ring buffer
 * @param c Character to store
 */
void ringBufferPutChar(RingBuffer *buffer, uint8_t c)
{
    int next = (buffer->head + 1) % RING_BUFFER_SIZE;
    if (next != buffer->tail) {
        buffer->buffer[buffer->head] = c;
        buffer->head = next;
    }
    // Note: Silently drops data if buffer is full
}

/**
 * @brief Put multiple characters into ring buffer
 * @param buffer Pointer to ring buffer
 * @param c Pointer to character array
 * @param length Number of characters to store
 */
void ringBufferPutChars(RingBuffer *buffer, const uint8_t *c, int length)
{
    for (int i = 0; i < length; i++) {
        int next = (buffer->head + 1) % RING_BUFFER_SIZE;
        if (next != buffer->tail) {
            buffer->buffer[buffer->head] = c[i];
            buffer->head = next;
        } else {
            // Stop if buffer is full to prevent overflow
            break;
        }
    }
}

/**
 * @brief Get a character from ring buffer
 * @param buffer Pointer to ring buffer
 * @param c Pointer to store retrieved character
 * @return true if character retrieved, false if buffer is empty
 */
bool ringBufferGetChar(RingBuffer *buffer, uint8_t *c)
{
    if (buffer->tail == buffer->head) {
        return false; // Buffer is empty
    }
    *c = buffer->buffer[buffer->tail];
    buffer->tail = (buffer->tail + 1) % RING_BUFFER_SIZE;
    return true;
}
/* ================================================================================================
 * INTERRUPT HANDLERS
 * ================================================================================================ */

/**
 * @brief UART0 interrupt service routine
 * 
 * Handles UART receive interrupts and timeout events.
 * Data is stored in ring buffer for processing in main loop.
 */
void am_uart_isr(void)
{
    uint32_t ui32Status, ui32Idle, receivedByte;
    uint8_t rxBuffer[UART_FIFO_MAX];

    // Service the FIFOs and clear interrupts
    am_hal_uart_interrupt_status_get(phUART, &ui32Status, true);
    am_hal_uart_interrupt_clear(phUART, ui32Status);
    am_hal_uart_interrupt_service(phUART, ui32Status, &ui32Idle);

    // Handle RX timeout and RX FIFO interrupts
    if (ui32Status & (AM_HAL_UART_INT_RX_TMOUT | AM_HAL_UART_INT_RX)) {
        uart_input(rxBuffer, &receivedByte);
        rxBuffer[receivedByte] = 0; // Null terminate for safety
        
        // Store received data in ring buffer
        ringBufferPutChars(&rxRingBuffer, rxBuffer, receivedByte);
    }
}

/* ================================================================================================
 * UART AND PERIPHERAL FUNCTIONS
 * ================================================================================================ */

/**
 * @brief Print string via UART
 * @param pcStr Null-terminated string to print
 */
void uart_print(char *pcStr)
{
    uint32_t ui32StrLen = 0;
    uint32_t ui32BytesWritten = 0;

    // Measure string length
    while (pcStr[ui32StrLen] != 0) {
        ui32StrLen++;
    }

    // Configure UART transfer
    const am_hal_uart_transfer_t sUartWrite = {
        .ui32Direction = AM_HAL_UART_WRITE,
        .pui8Data = (uint8_t *)pcStr,
        .ui32NumBytes = ui32StrLen,
        .ui32TimeoutMs = AM_HAL_UART_WAIT_FOREVER,
        .pui32BytesTransferred = &ui32BytesWritten,
    };

    CHECK_ERRORS(am_hal_uart_transfer(phUART, &sUartWrite));

    // Verify all bytes were transmitted
    if (ui32BytesWritten != ui32StrLen) {
        // Critical error: couldn't send the whole string
        while (1);
    }
}

/**
 * @brief Read data from UART FIFO
 * @param buf Buffer to store received data
 * @param len Pointer to store number of bytes read
 * @return Number of bytes read
 */
static uint32_t uart_input(uint8_t *buf, uint32_t *len)
{
    const am_hal_uart_transfer_t sUartRead = {
        .ui32Direction = AM_HAL_UART_READ,
        .pui8Data = buf,
        .ui32NumBytes = UART_FIFO_MAX, // Maximum FIFO read size
        .ui32TimeoutMs = 0,
        .pui32BytesTransferred = len,
    };

    am_hal_uart_transfer(phUART, &sUartRead);
    return *len;
}

/**
 * @brief Initialize RAK3183 LED pins
 * Sets LED1 and LED2 as outputs with initial HIGH state (LEDs off)
 */
static void init_rak3183_led(void)
{
    hal_gpio_init_out(LED1, 0); // LED1 off (active low)
    hal_gpio_init_out(LED2, 0); // LED2 off (active low)
}

/**
 * @brief Error handler for HAL function failures
 * @param ui32ErrorStatus Error status code from HAL function
 */
static void error_handler(uint32_t ui32ErrorStatus)
{
    ui32LastError = ui32ErrorStatus;
    // Add error handling logic here (e.g., reset, logging)
    while (1); // Halt on error for now
}

/* ================================================================================================
 * MAIN FUNCTION
 * ================================================================================================ */

/**
 * @brief Main application entry point
 * 
 * Initializes system clock, UART, peripherals and enters main processing loop.
 * The main loop runs the LoRaWAN modem engine and processes serial input.
 * 
 * @return Never returns (infinite loop)
 */
int main(void)
{
    /* ========================================================================================
     * SYSTEM INITIALIZATION
     * ======================================================================================== */
    
    // Set maximum system clock frequency
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

    // Configure and enable cache with default settings
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    // Start 32KHz crystal oscillator
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);
    
    // Wait for crystal to stabilize
    am_util_delay_ms(100);

    // Enable HFADJ (High Frequency Adjustment)
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_HFADJ_ENABLE, 0);

    /* ========================================================================================
     * UART INITIALIZATION
     * ======================================================================================== */
    
    // Initialize UART0 peripheral
    CHECK_ERRORS(am_hal_uart_initialize(0, &phUART));
    CHECK_ERRORS(am_hal_uart_power_control(phUART, AM_HAL_SYSCTRL_WAKE, false));

    // Set UART clock speed
    am_hal_uart_clock_speed_e eUartClockSpeed = eUART_CLK_SPEED_DEFAULT;
    CHECK_ERRORS(am_hal_uart_control(phUART, AM_HAL_UART_CONTROL_CLKSEL, &eUartClockSpeed));
    CHECK_ERRORS(am_hal_uart_configure(phUART, &g_sUartConfig));

    // Configure UART GPIO pins
    const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_COM_UART_TX0 = {
        .uFuncSel = AM_HAL_PIN_39_UART0TX,
    };
    const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_COM_UART_RX0 = {
        .uFuncSel = AM_HAL_PIN_40_UART0RX
    };
    
    am_hal_gpio_pinconfig(39, g_AM_BSP_GPIO_COM_UART_TX0); // TX pin
    am_hal_gpio_pinconfig(40, g_AM_BSP_GPIO_COM_UART_RX0); // RX pin

    /* ========================================================================================
     * INTERRUPT CONFIGURATION
     * ======================================================================================== */
    
    // Configure and enable UART interrupt
    NVIC_SetPriority(UART0_IRQn, 7);
    NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn));
    am_hal_interrupt_master_enable();

    /* ========================================================================================
     * PERIPHERAL AND STACK INITIALIZATION
     * ======================================================================================== */
    
    // Set stdio to use UART print function
    am_util_stdio_printf_init(uart_print);
    
    // Print startup banner
    am_util_stdio_printf("\r\nRAKwireless RAK3183\r\n");
    am_util_stdio_printf("------------------------------------------------------\r\n");
    am_util_stdio_printf("Version: %s\r\n", VERSION);

    // Initialize peripherals and protocol stack
    init_rak3183_led();
    lorawan_init();
    i2c_init();

    /* ========================================================================================
     * MAIN PROCESSING LOOP
     * ======================================================================================== */
    
    uint8_t character = 0;

    while (1) {
        // Run LoRaWAN modem engine (handles radio and protocol stack)
        smtc_modem_run_engine();

        // Process characters from ring buffer
        while (ringBufferGetChar(&rxRingBuffer, &character)) {
            // Echo character back to terminal
            am_util_stdio_printf("%c", character);
            
            // Process AT command input
            process_serial_input(character);
        }

        // Note: Deep sleep disabled for continuous operation
        // am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    }
}
