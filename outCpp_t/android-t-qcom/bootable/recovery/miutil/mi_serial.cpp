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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <cutils/properties.h>

#include "miutil/mi_serial.h"

int get_miphone_sn(char *code) {
    printf("Get serial number from miphone.\n");
    FILE *fp = fopen(SN_PATH, "r");
    if (fp == NULL) {
        printf("Open /proc/serial_num failed by %s ,retry from prop.\n",
            strerror(errno));
        property_get("ro.boot.cpuid", code, "n/a");
        if (!strncmp(code, "n/a", 3)) {
            printf("Error: Cannot get miphone serial\n");
            return -1;
        }
        return 0;
    }
    int c, i = 0;
    while ((c = fgetc(fp)) != EOF && c != '\n') {
        code[i++] = c;
    }
    fclose(fp);
    return 0;
}


int get_longcheer_sn(char *code) {
    printf("Get serial number from longcheer.\n");
    int fp, index, io_ret;
    int n = 0;
    struct otp_ctl otpctl;
    unsigned int high_bits,low_bits;
    char charindex[] = "0123456789abcdef";

    fp = open(LONGCHEER_SN_PATH, O_RDONLY, 0);
    if(fp == -1) {
        printf("Error: Cannot open the /dev/otp \n");
        return -1;
    }
    ioctl(fp, (unsigned int)OTP_GET_LENGTH, (unsigned long) &otpctl);
    otpctl.BufferPtr =(char *) malloc(sizeof(char)*3000);
    if (otpctl.BufferPtr == NULL) {
        printf("otpctl.BufferPtr malloc blk fail\n");
        close(fp);
        return -1;
    }

    otpctl.Length = 3000;
    otpctl.Offset = 0;
    otpctl.Status = 0;

    for(index=0; index<3000; index++)
        otpctl.BufferPtr[index]= 0xff;

    io_ret = ioctl(fp, (unsigned int)OTP_READ, (unsigned long) &otpctl);
    if(!otpctl.Status) {
        for(index=0; index<8; index++) {
            high_bits = otpctl.BufferPtr[index]/16;
            low_bits = otpctl.BufferPtr[index]%16;
            code[n++]=charindex[low_bits];
            code[n++]=charindex[high_bits];
        }
        code[15] = '\0';
        printf("otpvalue=%s\n", code);
    } else {
        printf("Recovery OTP: OTP operation error ! Status = %d\n", otpctl.Status);
        free(otpctl.BufferPtr);
        close(fp);
        return -1;
    }

    free(otpctl.BufferPtr);
    close(fp);
    return 0;
}
