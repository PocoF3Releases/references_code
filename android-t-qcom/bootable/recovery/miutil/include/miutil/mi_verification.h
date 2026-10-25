/**
 * Copyright (c) XiaoMi Mobile.  All Rights Reserved.
 *
 * mi_verification.h
 *
 * APIs for check encrypt message from server side.
 *
 * @author Anson Zhang <zhangyang_b@xiaomi.com>
 * @version 1.0
 */

#ifndef _RECOVERY_MI_VERIFICATION_H
#define _RECOVERY_MI_VERIFICATION_H

#include <android-base/logging.h>
#include "otautil/sysutil.h"
#include <setjmp.h>

#define PERM_LEN 256
#define VERSION_KEY "ro.build.version.incremental"
#define ROM_ZONE_KEY "ro.rom.zone"

#ifdef __cplusplus
extern "C" {
#endif

static jmp_buf jb;
void sig_bus(int sig);
int check_identity(const char *msg,  MemMapping* pMap);
int check_verify(const char *msg, MemMapping* pMap);
#ifdef __cplusplus
}
#endif

#endif /* _RECOVERY_MI_VERIFICATION_H */
