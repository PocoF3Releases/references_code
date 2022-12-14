 /*
 * Copyright (C) 2007 The Android Open Source Project
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

#define VERSION_KEY "ro.build.version.incremental"
#define CODEBASE_KEY "ro.build.version.release"
#define DEVICE_KEY "ro.product.device"
#define MOD_DEVICE_KEY "ro.product.mod_device"
#define TYPE_KEY "ro.build.type"
#define LOCALE_KEY "ro.product.locale"
#define CARRIER_KEY "ro.carrier"
#define VIRTUAL_AB_ENABLED_KEY "ro.virtual_ab.enabled"
#define SERIAL_KEY "ro.serialno"
#define ROM_ZONE_KEY "ro.rom.zone"
#define LOCALE_FILE "/cache/recovery/last_locale"
#define LAST_TRAP_FILE "/cache/recovery/last_trap"

extern FILE *g_cmd_pipe;
extern FILE *from_father;

void read_from_father(int fd);
void sideload_verification(const char* saveptr);
int service_mi_fd(int (*create_thread)(void (*)(int,  const std::string&), const std::string&), const char *name);