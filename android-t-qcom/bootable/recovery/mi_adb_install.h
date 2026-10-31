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

#ifndef _MI_ADB_INSTALL_H
#define _MI_ADB_INSTALL_H

#include <stdbool.h>

static const char *STATUS_FILE = "/cache/recovery/last_status";

/* adb_install.cpp */
void set_usb_driver(bool enabled);
void stop_adbd(RecoveryUI* ui);
// void maybe_restart_adbd();
void save_install_status(int new_status);

#define WAIT_COMMENT_TIMEOUT_SEC    60
#define ADB_INSTALL_TIMEOUT 300

/* mi_adb_install.cpp */
Device::BuiltinAction prompt_and_wait_adb_install(RecoveryUI* h, const char* install_file);

#endif /* _MI_ADB_INSTALL_H */