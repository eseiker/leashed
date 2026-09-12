#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FURI_HAL_BT_HCI_ABI 2U
#define FURI_HAL_BT_HCI_FRAME_MAX 260U

#ifdef __cplusplus
extern "C" {
#endif

/* One external host owns the controller. No callbacks point into FAP memory. */
uint32_t furi_hal_bt_hci_get_abi(void);
bool furi_hal_bt_hci_acquire(uint32_t abi);
bool furi_hal_bt_hci_send(const uint8_t* frame, size_t length, uint32_t timeout_ms);
/* Returns frame length, zero on timeout, or -1 on a latched transport fault. */
int32_t furi_hal_bt_hci_receive(uint8_t* frame, size_t capacity, uint32_t timeout_ms);
/* Resets the controller before releasing ownership. Failure requires reboot. */
bool furi_hal_bt_hci_release(void);

#ifdef __cplusplus
}
#endif
