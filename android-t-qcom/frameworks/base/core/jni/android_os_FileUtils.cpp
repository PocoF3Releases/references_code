/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <android-base/macros.h>
#include <nativehelper/JNIHelp.h>
#include <unistd.h>

#include "jni.h"
#include "core_jni_helpers.h"

namespace android {

static void android_os_FileUtils_syncAll(JNIEnv* env ATTRIBUTE_UNUSED,
                                         jobject clazz ATTRIBUTE_UNUSED)
{
    sync();
}

// ----------------------------------------------------------------------------

static const JNINativeMethod gMethods[] = {
    {"nativeSyncAll", "()V", (void*)android_os_FileUtils_syncAll},
};

int register_android_os_FileUtils(JNIEnv* env)
{
    return RegisterMethodsOrDie(env, "android/os/FileUtils", gMethods, NELEM(gMethods));
}

};
