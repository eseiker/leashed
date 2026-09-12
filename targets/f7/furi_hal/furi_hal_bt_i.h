#pragma once

#include <stdbool.h>

/* Internal transitions used by the bounded raw HCI controller bridge. */
bool furi_hal_bt_enter_ll_only(void);

/* Restarts BLE Full in its ordinary LL-host mode, without starting a profile. */
bool furi_hal_bt_leave_ll_only(void);
