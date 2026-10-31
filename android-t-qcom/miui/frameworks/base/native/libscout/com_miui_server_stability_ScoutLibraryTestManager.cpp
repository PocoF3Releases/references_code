#ifndef LOG_TAG
#define LOG_TAG "MIUIScout LibraryTest"
#endif

#include "jni.h"
#include "core_jni_helpers.h"
#include <stdio.h>
#include <string.h>
#include "utils/Log.h"
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <sys/resource.h>

#include <android-base/stringprintf.h>
#include <android-base/file.h>
#include <android-base/properties.h>

using android::base::GetBoolProperty;
using android::base::SetProperty;

using namespace android;

#define PROPERTIES_GKI_DEVICE  "persist.sys.device_config_gki"
#define PROPERTIES_ENABLE_MIUI_COREDUMP "persist.sys.enable_miui_coredump"

static void com_miui_server_stability_ScoutLibraryTestManager_enableCoreDump(JNIEnv *env, jobject clazz) {
    // core-pattern
    if (GetBoolProperty(PROPERTIES_GKI_DEVICE, false) || GetBoolProperty(PROPERTIES_ENABLE_MIUI_COREDUMP, false)) {
        SetProperty("sys.miui.gki_ndcd", "on");
    } else {
        SetProperty("sys.miui.ndcd", "on");
    }

    // process limits
    rlimit rl;
    getrlimit(RLIMIT_CORE, &rl);
    rl.rlim_cur = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);
}

static const JNINativeMethod methods[] = {
    {"enableCoreDump", "()V", (void *) com_miui_server_stability_ScoutLibraryTestManager_enableCoreDump},
};

static const char* const kClassPathName = "com/miui/server/stability/ScoutLibraryTestManager";

jboolean register_com_miui_server_stability_ScoutLibraryTestManager(JNIEnv* env) {
    jclass clazz = env->FindClass(kClassPathName);
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
