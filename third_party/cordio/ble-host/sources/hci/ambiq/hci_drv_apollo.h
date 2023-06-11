//*****************************************************************************
//
//! @file hci_drv_apollo.h
//!
//! @brief Additional header information for the Apollo implementation of HCI.
//
//*****************************************************************************

#ifndef HCI_DRV_APOLLO_H
#define HCI_DRV_APOLLO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Errors
//
//*****************************************************************************
#define HCI_DRV_SPECIFIC_ERROR_START   0x09000000
typedef enum
{
    HCI_DRV_TRANSMIT_QUEUE_FULL = HCI_DRV_SPECIFIC_ERROR_START,
    HCI_DRV_TX_PACKET_TOO_LARGE,
    HCI_DRV_RX_PACKET_TOO_LARGE,
    HCI_DRV_BLE_STACK_UNABLE_TO_ACCEPT_PACKET,
    HCI_DRV_PACKET_TRANSMIT_FAILED,
    HCI_DRV_IRQ_STUCK_HIGH,
    HCI_DRV_TOO_MANY_PACKETS,
}
hci_drv_error_t;

typedef void (*hci_drv_error_handler_t)(uint32_t ui32Error);

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern void HciDrvUartEnable(void);
extern void HciDrvUartDisable(void);
extern void HciDrvUartFlowOff(void);
extern void HciDrvUartFlowOn(void);
extern void HciDrvUartPause(void);
extern void HciDrvUartUnpause(void);
extern bool HciDrvUartSafeShutdown(void);

#if defined(AM_PART_APOLLO4) || defined(AM_PART_APOLLO4B)
extern uint32_t HciDrvRadioBoot(bool bColdBoot);
#else
#if defined(AM_PART_APOLLO3) || defined(AM_PART_APOLLO3P)
extern uint32_t HciDrvRadioBoot(bool bColdBoot);
#else
extern void HciDrvRadioBoot(uint32_t ui32UartModule);
#endif
#endif

extern void HciDrvRadioShutdown(void);
extern void HciDrvUartISR(uint32_t ui32Status);
extern bool_t HciDataReadyISR(void);
#if defined(AM_PART_APOLLO4) || defined(AM_PART_APOLLO4B)
#else
extern void HciDrvIntService(void);
#endif
extern void HciDrvGPIOService(void);
extern void HciDrvHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);
extern void HciDrvErrorHandlerSet(hci_drv_error_handler_t pfnErrorHandler);

#ifdef __cplusplus
};
#endif

#endif // HCI_DRV_APOLLO_H
