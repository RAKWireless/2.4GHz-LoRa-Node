/**
 * @file lorawan_app.c
 * @brief LoRaWAN application implementation for RAK3183
 * @version 1.1.0
 * @date 2025-07-02
 * 
 * This file contains the main LoRaWAN application logic including:
 * - Parameter management and flash storage
 * - Event handling and processing
 * - Sensor data collection and transmission
 * - Device initialization and configuration
 */

#include "lorawan_app.h"
#include "am_mcu_apollo.h"
#include "am_util.h"

#include "smtc_modem_api.h"
#include "smtc_modem_utilities.h"

#include "smtc_modem_hal.h"
#include "smtc_hal_dbg_trace.h"

#include "ralf_sx128x.h"

#include "smtc_hal_mcu.h"
#include "smtc_hal_gpio.h"

#include "hal/am_hal_gpio.h"
#include "smtc_modem_test_api.h"
#include "smtc_modem_utilities.h"
#include "smtc_hal_flash.h"
#include "lr1mac_defs.h"

#include "i2c.h"
#include "at_cmd.h"

/* ================================================================================================
 * CONSTANTS AND DEFINITIONS
 * ================================================================================================ */
#define LED1 44                     ///< LED1 GPIO pin number
#define LED2 45                     ///< LED2 GPIO pin number

#define ADDR_FLASH_AT_PARAM_CONTEXT (AM_HAL_FLASH_INSTANCE_SIZE + (4 * AM_HAL_FLASH_PAGE_SIZE))
#define MODEM_EXAMPLE_REGION SMTC_MODEM_REGION_WW2G4
#define STACK_ID 0

/**
 * @brief Auto-send configuration
 */
#define AUTO_SEND_PACKET_COUNT 100     // 自动发送包的总数
#define AUTO_JOIN_RETRY_INTERVAL 10     // 入网重试间隔（秒）

/**
 * @brief Range test configuration
 * Range test mode is now configurable via AT+RANGETEST command
 */
#define RANGE_TEST_PAYLOAD_SIZE 100     // Range test包大小（字节）

/**
 * @brief Default LoRaWAN credentials (all zeros - should be configured via AT commands)
 */
#define USER_LORAWAN_DEVICE_EUI    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define USER_LORAWAN_JOIN_EUI      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define USER_LORAWAN_APP_KEY       {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define USER_LORAWAN_NWKSKEY       {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define USER_LORAWAN_APPSKEY       {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

/* ================================================================================================
 * GLOBAL VARIABLES
 * ================================================================================================ */

/**
 * @brief LoRaWAN parameters with default values
 * @note These parameters are stored in flash and can be modified via AT commands
 */
volatile LoRaWAN_Params lora_params = {
    .dev_eui                = USER_LORAWAN_DEVICE_EUI, // 8 bytes
    .join_eui               = USER_LORAWAN_JOIN_EUI,   // 8 bytes
    .app_key                = USER_LORAWAN_APP_KEY,    // 16 bytes
    .nwkskey                = USER_LORAWAN_NWKSKEY,    // 16 bytes
    .appskey                = USER_LORAWAN_APPSKEY,    // 16 bytes
    .devaddr                = 0,

    .class                  = 0,
    .dr                     = 2,
    .confirm                = 1,
    .retry                  = 7,

    .interval               = 0,

    .join_mode              = 1,
    .nwm                    = 1,
    .range_test_enabled     = 0,
    .tx_power_dbm           = 13,

    .frequency_hz           = 2402000000,

    .sf                     = 1,
    .bw                     = 3,
    .cr                     = 0,
    .preamble_size          = 14,

    .auto_send_interval_sec = 5,

    .crc                    = 0
};

/**
 * @brief Received payload buffer and size
 */
uint8_t rx_payload_size;
uint8_t rx_payload[256];

/**
 * @brief Auto-send state management
 */
static struct {
    bool auto_send_enabled;           // 是否启用自动发送
    uint16_t packets_sent;           // 已发送包数量
    uint16_t target_packet_count;    // 目标发送包数量
    bool network_joined;             // 网络加入状态
    bool auto_join_enabled;          // 是否启用自动入网
} auto_send_state = {
    .auto_send_enabled = true,       // 上电自动启用
    .packets_sent = 0,
    .target_packet_count = AUTO_SEND_PACKET_COUNT,
    .network_joined = false,
    .auto_join_enabled = true        // 上电自动入网
};

/**
 * @brief Radio abstraction layer instance
 */
#if defined(SX128X)
const ralf_t modem_radio = RALF_SX128X_INSTANTIATE(NULL);
#elif defined(SX126X)
const ralf_t modem_radio = RALF_SX126X_INSTANTIATE(NULL);
#elif defined(LR11XX)
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE(NULL);
#else
#error "Please select radio board.."
#endif

/* ================================================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ================================================================================================ */

static uint16_t calculate_crc_for_lorawan_params(const LoRaWAN_Params *params);
static void get_event(void);
static const char* get_window_str(smtc_modem_event_downdata_window_t window);
static void range_test_uplink(void);
static void led_init(void);
static void led_on(uint8_t led_pin);
static void led_off(uint8_t led_pin);
static void led_blink(uint8_t led_pin, uint32_t duration_ms);



/* ================================================================================================
 * PARAMETER MANAGEMENT FUNCTIONS
 * ================================================================================================ */

/**
 * @brief Save LoRaWAN parameters to flash memory
 * @note Calculates CRC and writes parameters to flash for persistence
 */
void save_lora_params(void)
{
    uint16_t crc = calculate_crc_for_lorawan_params(&lora_params);
    lora_params.crc = crc;

    // Erase flash page and write new parameters
    hal_flash_erase_page(ADDR_FLASH_AT_PARAM_CONTEXT, 1);
    hal_flash_write_buffer(ADDR_FLASH_AT_PARAM_CONTEXT, (uint8_t *)&lora_params, sizeof(lora_params));
}

/**
 * @brief Load LoRaWAN parameters from flash memory
 * @note Validates CRC and loads parameters, saves default if CRC check fails
 */
void load_lora_params(void)
{
    uint16_t crc;
    LoRaWAN_Params lora_params_temp;
    
    // Read parameters from flash
    hal_flash_read_buffer(ADDR_FLASH_AT_PARAM_CONTEXT, (uint8_t *)&lora_params_temp, sizeof(lora_params_temp));

    // Print first 16 bytes of loaded struct for debug
    //am_util_stdio_printf("[DEBUG] Loaded lora_params_temp (first 16 bytes): ");
    // for (int i = 0; i < 16; i++) {
    //     am_util_stdio_printf("%02X ", ((uint8_t*)&lora_params_temp)[i]);
    // }
    // am_util_stdio_printf("\r\n");

    // Validate CRC
    crc = calculate_crc_for_lorawan_params(&lora_params_temp);
    //am_util_stdio_printf("[DEBUG] lora_params_temp.crc: 0x%04X, calculated CRC: 0x%04X\r\n", lora_params_temp.crc, crc);
    
    if (lora_params_temp.crc != crc) {
        am_util_stdio_printf("[DEBUG] CRC mismatch, loading defaults and saving to flash.\r\n");
        // CRC mismatch - save default parameters
        save_lora_params();
    } else {
        //am_util_stdio_printf("[DEBUG] CRC valid, using loaded parameters.\r\n");
        // CRC valid - use loaded parameters
        memcpy(&lora_params, &lora_params_temp, sizeof(lora_params));
    }
}

/* ================================================================================================
 * UTILITY FUNCTIONS
 * ================================================================================================ */

/**
 * @brief Convert downlink window enum to string
 * @param window Downlink window type
 * @return String representation of the window type
 */
static const char* get_window_str(smtc_modem_event_downdata_window_t window)
{
    switch (window) {
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RX1:         return "RX1";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RX2:         return "RX2";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXC:         return "RXC";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXC_MC_GRP0: return "RXC_MC_GRP0";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXC_MC_GRP1: return "RXC_MC_GRP1";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXC_MC_GRP2: return "RXC_MC_GRP2";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXC_MC_GRP3: return "RXC_MC_GRP3";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXB:         return "RXB";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXB_MC_GRP0: return "RXB_MC_GRP0";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXB_MC_GRP1: return "RXB_MC_GRP1";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXB_MC_GRP2: return "RXB_MC_GRP2";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXB_MC_GRP3: return "RXB_MC_GRP3";
        case SMTC_MODEM_EVENT_DOWNDATA_WINDOW_RXBEACON:    return "RXBEACON";
        default:                                           return "UNKNOWN";
    }
}

/* ================================================================================================
 * EVENT HANDLING FUNCTIONS
 * ================================================================================================ */

/**
 * @brief Process LoRaWAN modem events
 * @note This function is called by the modem when events occur
 */
static void get_event(void)
{
    smtc_modem_event_t current_event;
    uint8_t event_pending_count;
    uint8_t stack_id = STACK_ID;

    // Process all pending events
    do {
        // Read modem event
        smtc_modem_get_event(&current_event, &event_pending_count);

        switch (current_event.event_type) {
            case SMTC_MODEM_EVENT_RESET:
                // Handle modem reset - enter test mode if P2P mode is selected
                if (lora_params.nwm == 0) {
                    smtc_modem_test_start();
                } else {
                    // LoRaWAN模式，重置发送计数器并开始自动入网
                    auto_send_state.packets_sent = 0;
                    auto_send_state.network_joined = false;
                    
                if (lora_params.range_test_enabled) {
                    // Range Test模式：启用自动发送和自动入网
                    am_util_stdio_printf("+EVT:RESET:AUTO_SEND_ENABLED:%d_PACKETS\r\n", 
                                       auto_send_state.target_packet_count);
                    
                    // 如果启用自动入网，开始入网流程
                    if (auto_send_state.auto_join_enabled && lora_params.join_mode == 1) {
                        smtc_modem_join_network(STACK_ID);
                        am_util_stdio_printf("+EVT:JOIN_STARTED\r\n");
                    }
                } else {
                    // 传感器数据模式：禁用自动发送和自动入网
                    auto_send_state.auto_send_enabled = false;
                    auto_send_state.auto_join_enabled = false;
                }
                }
                break;

            case SMTC_MODEM_EVENT_ALARM:
                SMTC_HAL_TRACE_INFO("Event received: ALARM\r\n");
                
                if (lora_params.range_test_enabled) {
                    // Range Test模式：检查是否需要继续自动发送或重试入网
                    if (auto_send_state.auto_send_enabled && 
                        auto_send_state.network_joined &&
                        auto_send_state.packets_sent < auto_send_state.target_packet_count) {
                        
                        // 发送数据包 - Range Test模式
                        range_test_uplink();
                        auto_send_state.packets_sent++;
                        
                        am_util_stdio_printf("+EVT:AUTO_SEND:PACKET_%d_OF_%d\r\n", 
                                           auto_send_state.packets_sent, 
                                           auto_send_state.target_packet_count);
                        
                    } else if (auto_send_state.auto_join_enabled && 
                              !auto_send_state.network_joined && 
                              lora_params.join_mode == 1) {
                        
                        // 重试入网
                        smtc_modem_join_network(STACK_ID);
                        am_util_stdio_printf("+EVT:JOIN_RETRY\r\n");
                    }
                }
                break;

            case SMTC_MODEM_EVENT_JOINED:
                am_util_stdio_printf("+EVT:JOINED\r\n");

                //测试发现重传必须在入网之后设置才会生效
                lorawan_api_dr_strategy_set(USER_DR_DISTRIBUTION);
                smtc_modem_set_nb_trans(STACK_ID, lora_params.retry);

                led_blink(LED1, 1000);
                /* 发送一个空包 */
                // uint8_t empty_payload[1] = {0};
                // smtc_modem_request_uplink(STACK_ID, 1, 0, empty_payload, 0);
                // am_util_stdio_printf("+EVT:JOINED:SEND_EMPTY_PACKET\r\n");
                
                // 打印当前确认和重传参数
                am_util_stdio_printf("+EVT:JOIN_CONFIG:CONFIRM=%d:RETRY=%d\r\n", 
                                   lora_params.confirm, lora_params.retry);
                
                if (lora_params.range_test_enabled) {
                    auto_send_state.network_joined = true;
                    // Range Test模式：入网后启动定时器，按正常间隔发送
                    if (auto_send_state.auto_send_enabled && 
                        auto_send_state.packets_sent < auto_send_state.target_packet_count) {
                        
                        am_util_stdio_printf("+EVT:AUTO_SEND_STARTING:INTERVAL_%dSEC:TARGET_%d_PACKETS\r\n", 
                                           lora_params.auto_send_interval_sec, auto_send_state.target_packet_count);
                        
                        // 启动正常间隔定时器
                        smtc_modem_alarm_start_timer(lora_params.auto_send_interval_sec);
                    }
                } else {
                    // 传感器数据模式：入网成功，但不启动自动发送
                    //am_util_stdio_printf("+EVT:JOINED:SENSOR_MODE:NO_AUTO_SEND\r\n");
                }
                // } else if (lora_params.interval > 0) {
                //     // 传统的间隔发送模式
                //     smtc_modem_alarm_start_timer(lora_params.interval);
                // }
                break;

            case SMTC_MODEM_EVENT_TXDONE:
                am_util_stdio_printf("+EVT:TX_DONE\r\n");

                // 检查是否已达到目标包数
                if (auto_send_state.packets_sent >= auto_send_state.target_packet_count) {
                    auto_send_state.auto_send_enabled = false;
                    am_util_stdio_printf("+EVT:AUTO_SEND_COMPLETED:TOTAL_%d_PACKETS\r\n", 
                                           auto_send_state.packets_sent);
                } else {
                    // 重新启动定时器继续发送
                    smtc_modem_alarm_start_timer(lora_params.auto_send_interval_sec);
                }

                led_blink(LED1, 100);
                // Report detailed transmission status
                switch (current_event.event_data.txdone.status) {
                    case SMTC_MODEM_EVENT_TXDONE_NOT_SENT:
                        am_util_stdio_printf("+EVT:TX_NOT_SENT\r\n");
                        break;
                    case SMTC_MODEM_EVENT_TXDONE_SENT:
                        if (lora_params.confirm == 1) {
                            am_util_stdio_printf("+EVT:SEND_CONFIRMED_FAILED\r\n");
                        }
                        break;
                    case SMTC_MODEM_EVENT_TXDONE_CONFIRMED:
                        am_util_stdio_printf("+EVT:SEND_CONFIRMED_OK\r\n");
                        // 发送数据包，LED2闪烁200ms
                        led_blink(LED2, 100);
                        int16_t snr = lorawan_api_last_snr_get();
                        int16_t rssi = lorawan_api_last_rssi_get();
                        am_util_stdio_printf("+EVT:ACK_RECEIVED:RSSI=%d:SNR=%.1f\r\n", 
                                           rssi ,
                                           snr / 1.0f);

                        break;
                    default:
                        am_util_stdio_printf("+EVT:SEND_UNKNOWN_STATUS:%d\r\n", 
                                           current_event.event_data.txdone.status);
                        break;
                }
                break;

            case SMTC_MODEM_EVENT_DOWNDATA:
                // Handle received downlink data
                rx_payload_size = (uint8_t)current_event.event_data.downdata.length;
                memcpy(rx_payload, current_event.event_data.downdata.data, rx_payload_size);

                // Format and print downlink information
                am_util_stdio_printf("+EVT:%s:%d:%.2f:UNICAST:%u:",
                    get_window_str(current_event.event_data.downdata.window),
                    current_event.event_data.downdata.rssi - 64,
                    current_event.event_data.downdata.snr / 4.0f,
                    current_event.event_data.downdata.fport
                );
                
                // Print payload in hexadecimal format
                for (uint8_t i = 0; i < rx_payload_size; i++) {
                    am_util_stdio_printf("%02X", rx_payload[i]);
                }
                am_util_stdio_printf("\r\n");
                break;

            case SMTC_MODEM_EVENT_UPLOADDONE:
                am_util_stdio_printf("+EVT:UPLOADDONE\r\n");
                break;

            case SMTC_MODEM_EVENT_SETCONF:
                SMTC_HAL_TRACE_INFO("Event received: SETCONF\r\n");
                break;

            case SMTC_MODEM_EVENT_MUTE:
                SMTC_HAL_TRACE_INFO("Event received: MUTE\r\n");
                break;

            case SMTC_MODEM_EVENT_STREAMDONE:
                SMTC_HAL_TRACE_INFO("Event received: STREAMDONE\r\n");
                break;

            case SMTC_MODEM_EVENT_JOINFAIL:
                am_util_stdio_printf("+EVT:JOIN_FAILED_RX_TIMEOUT\r\n");
   
                // Leave network after join failure
                smtc_modem_leave_network(stack_id);
                
                // If range test mode is enabled and auto join is enabled, set retry timer
                if (lora_params.range_test_enabled && auto_send_state.auto_join_enabled && lora_params.join_mode == 1) {
                    am_util_stdio_printf("+EVT:JOIN_RETRY_IN_%d_SECONDS\r\n", AUTO_JOIN_RETRY_INTERVAL);
                    smtc_modem_alarm_start_timer(AUTO_JOIN_RETRY_INTERVAL);
                }
                break;

            case SMTC_MODEM_EVENT_TIME:
                SMTC_HAL_TRACE_INFO("Event received: TIME\r\n");
                break;

            case SMTC_MODEM_EVENT_TIMEOUT_ADR_CHANGED:
                SMTC_HAL_TRACE_INFO("Event received: TIMEOUT_ADR_CHANGED\r\n");
                break;

            case SMTC_MODEM_EVENT_NEW_LINK_ADR:
                SMTC_HAL_TRACE_INFO("Event received: NEW_LINK_ADR\r\n");
                break;

            case SMTC_MODEM_EVENT_LINK_CHECK:
                SMTC_HAL_TRACE_INFO("Event received: LINK_CHECK\r\n");
                break;

            case SMTC_MODEM_EVENT_ALMANAC_UPDATE:
                SMTC_HAL_TRACE_INFO("Event received: ALMANAC_UPDATE\r\n");
                break;

            case SMTC_MODEM_EVENT_USER_RADIO_ACCESS:
                SMTC_HAL_TRACE_INFO("Event received: USER_RADIO_ACCESS\r\n");
                break;

            case SMTC_MODEM_EVENT_CLASS_B_PING_SLOT_INFO:
                SMTC_HAL_TRACE_INFO("Event received: CLASS_B_PING_SLOT_INFO\r\n");
                break;

            case SMTC_MODEM_EVENT_CLASS_B_STATUS:
                SMTC_HAL_TRACE_INFO("Event received: CLASS_B_STATUS\r\n");
                break;

            default:
                SMTC_HAL_TRACE_ERROR("Unknown event %u\r\n", current_event.event_type);
                break;
        }
    } while (event_pending_count > 0);
}

/* ================================================================================================
 * INITIALIZATION FUNCTIONS
 * ================================================================================================ */

/**
 * @brief Initialize LoRaWAN application
 * @note Sets up hardware, modem, and configures parameters from flash
 */
void lorawan_init(void)
{
    // Initialize hardware peripherals
    hal_spi_init(0, 0, 0, 0);
    hal_rtc_init();
    hal_lp_timer_init();
    
    // Initialize LEDs
    led_init();
    
    // Initialize modem with radio and event handler
    hal_mcu_disable_irq();
    hal_mcu_init();
    smtc_modem_init(&modem_radio, &get_event);
    hal_mcu_enable_irq();
    
    // Configure modem region and power offset
    smtc_modem_set_region(STACK_ID, MODEM_EXAMPLE_REGION);
    smtc_modem_set_tx_power_offset_db(STACK_ID, lora_params.tx_power_dbm);

    // Load parameters from flash
    load_lora_params();

    // Display current working mode
    char *mode[2] = {"P2P", "LoRaWAN"};
    am_util_stdio_printf("Current Work Mode: %s\r\n", mode[lora_params.nwm]);
    
    // 显示测试模式
    // if (lora_params.range_test_enabled) {
    //     am_util_stdio_printf("Test Mode: Range Test (10-byte packets, 0-99 counter)\r\n");
    // } else {
    //     am_util_stdio_printf("Test Mode: Sensor Data Mode (Cayenne LPP)\r\n");
    // }

    // Configure activation mode based on join mode
    if (lora_params.join_mode == 0) {
        // ABP mode configuration
        lorawan_api_set_activation_mode(1);  // ABP mode parameter is 1
        am_util_stdio_printf("ABP mode\r\n");
        
        // Set device address and session keys
        lorawan_api_devaddr_set(lora_params.devaddr);
        
        int ret = smtc_modem_crypto_set_key(SMTC_SE_NWK_S_ENC_KEY, lora_params.nwkskey);
        if (ret) {
            SMTC_HAL_TRACE_ERROR("SMTC_SE_NWK_S_ENC_KEY ERROR\r\n");
        }
        
        ret = smtc_modem_crypto_set_key(SMTC_SE_APP_S_KEY, lora_params.appskey);
        if (ret) {
            SMTC_HAL_TRACE_ERROR("SMTC_SE_APP_S_KEY ERROR\r\n");
        }
    } else {
        // OTAA mode configuration
        smtc_modem_set_deveui(STACK_ID, lora_params.dev_eui);
        smtc_modem_set_joineui(STACK_ID, lora_params.join_eui);
        smtc_modem_set_nwkkey(STACK_ID, lora_params.app_key);
    }

    // Set device class
    uint8_t rc = smtc_modem_set_class(STACK_ID, lora_params.class);
    if (rc != SMTC_MODEM_RC_OK) {
        SMTC_HAL_TRACE_WARNING("smtc_modem_set_class failed: rc=(%d)\r\n", rc);
    } else {
        SMTC_HAL_TRACE_INFO("Device class set to: %d\r\n", lora_params.class);
    }

    // Configure data rate strategy and retransmission count
    lorawan_api_dr_strategy_set(USER_DR_DISTRIBUTION);
    uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {0};
    memset(custom_datarate, lora_params.dr, SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH);
    smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, custom_datarate);

    smtc_modem_set_crystal_error_ppm(40000);
}

// /* ================================================================================================
//  * DATA TRANSMISSION FUNCTIONS
//  * ================================================================================================ */

// /**
//  * @brief Collect sensor data and send LoRaWAN uplink
//  * @note Uses Cayenne LPP format for sensor data encoding
//  */
// void data_lpp_uplink(void)
// {
//     uint8_t buff_idx = 0;
//     int8_t buffer[24] = {0};

//     // 添加包计数器作为第一个数据（Channel 0，Digital Input）
//     if (auto_send_state.auto_send_enabled) {
//         buffer[buff_idx++] = 0;      // Channel 0
//         buffer[buff_idx++] = 0x00;   // LPP Digital Input type
//         buffer[buff_idx++] = auto_send_state.packets_sent + 1; // 当前包序号（从1开始）
//     }

//     // Add accelerometer data if available (LIS3DH sensor)
//     if (lis3dh_initialized == true) {
//         RAK1904_func();
//         buffer[buff_idx++] = 1;      // Channel 1
//         buffer[buff_idx++] = 0x71;   // LPP Accelerometer type

//         // X-axis data (16-bit)
//         buffer[buff_idx++] = (val[0]) >> 8;
//         buffer[buff_idx++] = val[0];

//         // Y-axis data (16-bit)
//         buffer[buff_idx++] = (val[1]) >> 8;
//         buffer[buff_idx++] = (val[1]);

//         // Z-axis data (16-bit)
//         buffer[buff_idx++] = (val[2]) >> 8;
//         buffer[buff_idx++] = (val[2]);
//     }

//     // Add temperature and humidity data if available (SHTC3 sensor)
//     if (shtc3_initialized == true) {
//         RAK1901_func();
        
//         // Temperature data
//         buffer[buff_idx++] = 2;      // Channel 2
//         buffer[buff_idx++] = 0x67;   // LPP Temperature type
//         buffer[buff_idx++] = (uint8_t)(val[3] >> 8);
//         buffer[buff_idx++] = (uint8_t)val[3];

//         // Humidity data
//         buffer[buff_idx++] = 2;      // Channel 2
//         buffer[buff_idx++] = 0x68;   // LPP Humidity type
//         buffer[buff_idx++] = (uint8_t)(val[4]);
//     }

//     // 如果没有传感器数据但有包计数器，确保至少发送包计数器
//     if (buff_idx == 0 && auto_send_state.auto_send_enabled) {
//         buffer[buff_idx++] = 0;      // Channel 0
//         buffer[buff_idx++] = 0x00;   // LPP Digital Input type
//         buffer[buff_idx++] = auto_send_state.packets_sent + 1; // 当前包序号
//     }

//     // Send data if any data was collected
//     if (buff_idx != 0) {
//         smtc_modem_request_uplink(STACK_ID, 1, true, buffer, buff_idx);
//     }
// }

/**
 * @brief Send range test uplink packet
 * @note Sends fixed 100-byte payload with packet counter from 0 to 99
 */
static void range_test_uplink(void)
{
    uint8_t buffer[RANGE_TEST_PAYLOAD_SIZE] = {0};  // 初始化为全0
    uint8_t packet_counter = auto_send_state.packets_sent; // 从0开始计数
    
    if (lora_params.range_test_enabled) {
        // 构建100字节的range test payload
        buffer[0] = 0x52;                           // 'R' - Range test identifier
        buffer[1] = 0x41;                           // 'A' - Range test identifier  
        buffer[2] = 0x4B;                           // 'K' - Range test identifier
        buffer[3] = (uint8_t)(packet_counter);      // 包计数器（0-99）
        buffer[4] = (uint8_t)(packet_counter >> 8); // 包计数器高字节（预留）
        buffer[5] = (uint8_t)(lora_params.auto_send_interval_sec);        // 上传间隔（秒）低字节
        buffer[6] = (uint8_t)(lora_params.auto_send_interval_sec >> 8);   // 上传间隔（秒）高字节
        // buffer[7-99] 保持为0（由初始化自动设置）

        if(lora_params.confirm)
        {
            smtc_modem_request_uplink(STACK_ID, 1, true, buffer, RANGE_TEST_PAYLOAD_SIZE);
        }
        else
        {
            smtc_modem_set_nb_trans(STACK_ID, 1);
            smtc_modem_request_uplink(STACK_ID, 1, false, buffer, RANGE_TEST_PAYLOAD_SIZE);
        }
        am_util_stdio_printf("\r\n+RANGE_TEST:PACKET_%d", packet_counter);
        am_util_stdio_printf("\r\n");
    }
}

/* ================================================================================================
 * CRC CALCULATION FUNCTIONS
 * ================================================================================================ */


/**
 * @brief CRC16 lookup table for fast calculation
 */
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/**
 * @brief Calculate CRC16 checksum
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return Calculated CRC16 value
 */
uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF; // Initialize CRC register

    while (length--) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ *data++) & 0xFF];
    }

    return crc;
}

/**
 * @brief Calculate CRC for LoRaWAN parameters structure
 * @param params Pointer to LoRaWAN parameters structure
 * @return Calculated CRC16 value (excludes the CRC field itself)
 */
static uint16_t calculate_crc_for_lorawan_params(const LoRaWAN_Params *params) 
{
    return crc16((const uint8_t *)params, sizeof(LoRaWAN_Params) - sizeof(params->crc));
}

/* ================================================================================================
 * LED CONTROL FUNCTIONS
 * ================================================================================================ */

/**
 * @brief Initialize LED GPIO pins
 */
static void led_init(void)
{
    // Configure LED1 pin as output
    am_hal_gpio_pinconfig(LED1, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(LED1, AM_HAL_GPIO_OUTPUT_CLEAR); // Turn off initially
    
    // Configure LED2 pin as output  
    am_hal_gpio_pinconfig(LED2, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(LED2, AM_HAL_GPIO_OUTPUT_CLEAR); // Turn off initially
}

/**
 * @brief Turn on LED
 * @param led_pin GPIO pin number of the LED
 */
static void led_on(uint8_t led_pin)
{
    am_hal_gpio_state_write(led_pin, AM_HAL_GPIO_OUTPUT_SET);
}

/**
 * @brief Turn off LED
 * @param led_pin GPIO pin number of the LED
 */
static void led_off(uint8_t led_pin)
{
    am_hal_gpio_state_write(led_pin, AM_HAL_GPIO_OUTPUT_CLEAR);
}

/**
 * @brief Blink LED for specified duration
 * @param led_pin GPIO pin number of the LED
 * @param duration_ms Duration in milliseconds to keep LED on
 */
static void led_blink(uint8_t led_pin, uint32_t duration_ms)
{
    // Turn on LED
    led_on(led_pin);
    
    // Simple delay (blocking) - for non-blocking, would need timer
    am_util_delay_ms(duration_ms);
    
    // Turn off LED
    led_off(led_pin);
}


