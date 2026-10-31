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
#ifndef __MI_SERIAL_H
#define __MI_SERIAL_H

 #include <sys/mount.h>

#define SN_PATH "/proc/serial_num"
#define LONGCHEER_SN_PATH "/dev/otp"

#define OTP_MAGIC 'k'
#define OTP_GET_LENGTH _IOW(OTP_MAGIC, 1, int)
#define OTP_READ _IOW(OTP_MAGIC, 2, int)

struct otp_ctl {
    unsigned int QLength;
    unsigned int Offset;
    unsigned int Length;
    char *BufferPtr;
    unsigned int Status;
};

#ifdef __cplusplus
extern "C" {
#endif

int get_miphone_sn(char *code);
int get_longcheer_sn(char *code);

#ifdef __cplusplus
}
#endif

#endif