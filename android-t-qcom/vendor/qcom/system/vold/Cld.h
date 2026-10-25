/*
 * Copyright (C) Xiaomi Inc.
 *
 */
#ifndef ANDROID_VOLD_CLD_H
#define ANDROID_VOLD_CLD_H

#include <android-base/logging.h>
#include <android-base/strings.h>
#include "android/os/IVoldTaskListener.h"

namespace android {
namespace vold {

#define CLD_NOTSUPPORT -1

//#ifdef MIUI_SUPPORT_CLD
void miuiSetCldListener(const android::sp<android::os::IVoldTaskListener>& listener);
int miuiRunCldOnHal();
int miuiGetFragLevelHal();
//#else
//static void miuiSetCldListener(const android::sp<android::os::IVoldTaskListener>& listener) {
//    return;
//}
//static int miuiRunCldOnHal() {
//    return CLD_NOTSUPPORT;
//
//}

//static int miuiGetFragLevelHal() {
//    return CLD_NOTSUPPORT;
//}
//#endif // MIUI_SUPPORT_CLD

} // namespace vold
} // namespace android

#endif  /* ANDROID_VOLD_CLD_H */
