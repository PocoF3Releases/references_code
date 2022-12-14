/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RECOVERY_MI_WIPE_H_
#define _RECOVERY_MI_WIPE_H_
#include "recovery_ui/ui.h"
#include "recovery_ui/device.h"

#ifdef __cplusplus
extern "C" {
#endif



int internal_erase_cache(RecoveryUI* ui);
/**
  * Wipe user data in /data and /cache.
  * Skip some directories in /data:
  *    "/data/miui"
  */
int internal_wipe_data_partial_erase_cache(RecoveryUI* ui);

int internal_erase_data_cache(Device* device, bool convert_fbe);
int internal_erase_storage(RecoveryUI* ui);

int should_wipe_efs_while_wipe_data();
int internal_wipe_efs(void);

#ifdef __cplusplus
}
#endif

#endif /* _RECOVERY_MI_WIPE_H_ */
