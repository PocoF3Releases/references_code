/*
* Copyright (C) 2014 MediaTek Inc.
* Modification based on code covered by the mentioned copyright
* and/or permission notice(s).
*/
/* //device/libs/android_runtime/android_util_Process.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "ActivityHelper"

#include <utils/Log.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <cutils/sched_policy.h>
#include <utils/String8.h>
#include <utils/Vector.h>


#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <signal.h>
#include <sys/errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <nativehelper/JNIHelp.h>

using namespace android;


static jlong getFreeMemoryImpl(const char* const sums[], const size_t sumsLen[], size_t num)
{
    int fd = open("/proc/meminfo", O_RDONLY);

    if (fd < 0) {
        ALOGW("Unable to open /proc/meminfo");
        return -1;
    }

    char buffer[512];
    const int len = read(fd, buffer, sizeof(buffer)-1);
    close(fd);

    if (len < 0) {
        ALOGW("Unable to read /proc/meminfo");
        return -1;
    }
    buffer[len] = 0;

    size_t numFound = 0;
    jlong mem = 0;

    char* p = buffer;
    while (*p && numFound < num) {
        int i = 0;
        while (sums[i]) {
            if (strncmp(p, sums[i], sumsLen[i]) == 0) {
                p += sumsLen[i];
                while (*p == ' ') p++;
                char* num = p;
                while (*p >= '0' && *p <= '9') p++;
                if (*p != 0) {
                    *p = 0;
                    p++;
                    if (*p == 0) p--;
                }
                mem += atoll(num) * 1024;
                numFound++;
                break;
            }
            i++;
        }
        p++;
    }

    return numFound > 0 ? mem : -1;
}

static jlong getIonCachedSize(void)
{
    FILE *fp;
    char line[1024];
    const char *ionFile = "/d/ion/heaps/system";
    const char *mtkIonFile = "/d/ion/ion_mm_heap";
    const char *realIonFile = NULL;
    unsigned long cachedSize = 0;

    if (access(ionFile, R_OK)) {
        ALOGW("Unable to access %s: %s", ionFile, strerror(errno));
        if (access(mtkIonFile, R_OK)) {
            ALOGW("Unable to access %s: %s", mtkIonFile, strerror(errno));
            return 0;
        } else {
            realIonFile = mtkIonFile;
        }
    } else {
        realIonFile = ionFile;
    }

    fp = fopen(realIonFile, "re");
    if (!fp) {
        ALOGW("Unable to open %s: %s", realIonFile, strerror(errno));
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "pool total", strlen("pool total")) == 0) {
            if (sscanf(line, "pool total (uncached + cached + secure) = %lu",
                &cachedSize) == 1) {
                break;
            }
            if (sscanf(line, "pool total (uncached + cached) = %lu",
                &cachedSize) == 1) {
                break;
            }
            if (sscanf(line, "pool total (%*[^)]) = %lu",
                &cachedSize) == 1) {
                break;
            }
        }
    }

    fclose(fp);

    return cachedSize;
}

static jlong android_os_Process_getFreeMemory(JNIEnv* env, jobject clazz)
{
    static const char* const sums[] = { "MemFree:", "Cached:", "Buffers:",
                                        "SReclaimable:", NULL };
    static const size_t sumsLen[] = { strlen("MemFree:"), strlen("Cached:"),
                                      strlen("Buffers:"), strlen("SRecliamable:"), 0 };
    return getFreeMemoryImpl(sums, sumsLen, 4);
}

static jlong android_os_Process_getTotalMemory(JNIEnv* env, jobject clazz)
{
    static const char* const sums[] = { "MemTotal:", NULL };
    static const size_t sumsLen[] = { strlen("MemTotal:"), 0 };
    return getFreeMemoryImpl(sums, sumsLen, 1);
}

static jlong android_os_Process_getCachedLostMemory(JNIEnv* env, jobject clazz)
{
    jlong cachedLostRam = 0;

    cachedLostRam += getIonCachedSize();

    return cachedLostRam;
}

static const JNINativeMethod methods[] = {
    {"getNativeFreeMemory", "()J", (void*)android_os_Process_getFreeMemory},
    {"getNativeCachedLostMemory", "()J", (void*)android_os_Process_getCachedLostMemory},
};

static const char* const kClassPathName = "com/android/server/am/MiuiActivityHelper";

jboolean register_MiuiActivityHelper(JNIEnv* env)
{
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class android.miui.Shell");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
