/******************************************************************************
 *
 *  Copyright (C) 2015 Google Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include <hardware/bluetooth.h>

#include "gatt_api.h"

typedef uint16_t tCONN_STATUS;

typedef enum {
  BTIF_DEBUG_LE_CONNECTED = 1,
  BTIF_DEBUG_LE_DISCONNECTED,
  BTIF_DEBUG_CLASSICAL_CONNECTED,
  BTIF_DEBUG_CLASSICAL_DISCONNECTED,
  BTIF_DEBUG_SCO_CONNECTED,
  BTIF_DEBUG_SCO_DISCONNECTED,
  BTIF_DEBUG_ESCO_CONNECTED,
  BTIF_DEBUG_ESCO_DISCONNECTED
} btif_debug_conn_state_t;

// Report a connection state change
void btif_debug_conn_state(const RawAddress& bda,
                           const btif_debug_conn_state_t state,
                           const tCONN_STATUS conn_status);

void btif_debug_conn_dump(int fd);
