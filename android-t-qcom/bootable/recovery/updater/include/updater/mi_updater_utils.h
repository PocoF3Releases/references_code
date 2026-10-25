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

#ifndef _MI_UPDATER_UTILS_H_
#define _MI_UPDATER_UTILS_H_

#include <time.h>

#ifdef MTK_VENDOR
#include "edify/expr.h"
#include "mt_updater.h"
#endif //MTK_VENDOR

void print_device_info(const char *package, time_t start);
void print_filesystem_info(const char *device_name, const char *fs_type);
void cleanup(int updater_result);

#endif /* _MI_UPDATER_UTILS_H_ */
