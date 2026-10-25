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

#ifndef _RECOVERY_MI_BATTERY_H_
#define _RECOVERY_MI_BATTERY_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_LEVEL_CHARGING_LOW    8
#define BATTERY_LEVEL_LOW       15
#define BATTERY_LEVEL_NORMAL    30
#define BATTERY_LEVEL_FULL      90

struct battery_state {
    int is_charging;
    int level;
    int online;
};

struct battery_state read_battery_state(void);

#ifdef __cplusplus
}
#endif

#endif /* _RECOVERY_MI_BATTERY_H_ */
