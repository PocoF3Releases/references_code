#define LOG_TAG "octvm_runtime"

#include <stdio.h>
#include <errno.h>

#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/PermissionCache.h>
#include <private/android_filesystem_config.h>
#include <cutils/properties.h>

extern "C" {
#include "klo_internal.h"
}
#include "anonymous_id.h"

#include "octvm/AndroidBridge.h"
#include "octvm/SystemKloProxy.h"

namespace android {
//dump permission
static const String16 sDump("android.permission.DUMP");

const String8 SystemKloProxy::SName = String8("miui.whetstone.klo");
const String8 SystemKloProxy::FeaturePropKey = String8("persist.sys.klo");

SystemKloProxy::SystemKloProxy()
{
    addDefaultMonitor();
    if (isFeatureOn()) {
        startKloMonitor();
    }
}

SystemKloProxy::~SystemKloProxy(){}

const char* SystemKloProxy::getServiceName() {
    return SName;
}

bool SystemKloProxy::isServiceStarted() {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->checkService(String16(SName));
    if (binder != 0) {
        ALOGI("miui.whetstone.klo is already started");
        return true;
    } else {
        ALOGI("miui.whetstone.klo is not started");
        return false;
    }
}

bool SystemKloProxy::isFeatureOn()
{
    char feature_status[PROPERTY_VALUE_MAX];
    if (property_get(FeaturePropKey.string(), feature_status, "")) {
        if (strcmp(feature_status, "on") == 0)
            return true;
    }
    return false;
}

void SystemKloProxy::setFeatureStatus(char *status) {
    property_set(FeaturePropKey.string(), status);
}

void SystemKloProxy::getFeatureStatus(char *status) {
    property_get(FeaturePropKey.string(), status, "");
}

int32_t SystemKloProxy::klo_feature_enable(int enable) {
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }
    return FeatureEnable(enable);
}

int32_t SystemKloProxy::register_klo_task(klo_task_t *task) {
    //calling into liboctvm
    return klo_register_klo_task(task);
}

int32_t SystemKloProxy::unregister_klo_task(int32_t task_id) {
    //calling into liboctvm
    return klo_unregister_klo_task(task_id);
}

int32_t SystemKloProxy::set_klo_dump_files(const Vector<String16>& fileDescs) {
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    Vector<const char *> *file_descs = new Vector<const char *>();
    for (int i = 0; i < fileDescs.size(); i++) {
        file_descs->add(strdup(String8(fileDescs.itemAt(i)).string()));
    }
    klo_set_dump_files(file_descs->size(), file_descs->editArray());
    for (int i = 0; i < file_descs->size(); i++) {
        free((char*)file_descs->itemAt(i));
    }
    delete file_descs;
    return 0;
}

int32_t SystemKloProxy::klo_system_debug(const Vector<String16>& args) {
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid > 10000) { //should not called by third-party app
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    int argc = args.size();
    if (argc == 2 && args.itemAt(0) == String16("wake_lock")) {
        ALOGD("wake_lock %s", String8(args.itemAt(1)).string());
        acquire_kernel_wakelock(String8(args.itemAt(1)).string());
    }
    else if (argc == 2 && args.itemAt(0) == String16("wake_unlock")) {
        ALOGD("wake_lock %s", String8(args.itemAt(1)).string());
        release_kernel_wakelock(String8(args.itemAt(1)).string());
    }
    else if (argc == 2 && args.itemAt(0) == String16("set_brightness")) {
        int brightness = atoi(String8(args.itemAt(1)).string());
        ALOGD("set_brightness %d", brightness);
        set_lcd_backlight_brightness(brightness);
    }
    return 0;
}

int32_t SystemKloProxy::klo_dump_system(const char *file_path) {
    FILE* dest_fp = fopen(file_path, "w");
    if (dest_fp == NULL) {
        ALOGE("cannot open the file path %s: %s.", file_path, strerror(errno));
        return -1;
    }
    dup2(fileno(dest_fp), fileno(stdout));
    dumpstate_silent();
    fclose(dest_fp);
    return 0;
}

int32_t SystemKloProxy::klo_get_anonymous_id(String16& anonymous_id) {
    char *mid = get_local_mid();
    if (mid != NULL) {
        anonymous_id = String16(mid);
    }
    return 0;
}

int32_t SystemKloProxy::dump(int fd, const Vector<String16>& args) {
    String8 result;
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    pid_t callingPid = IPCThreadState::self()->getCallingPid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM && callingUid != AID_SHELL &&
            !PermissionCache::checkPermission(sDump, callingPid, callingUid)) {
        result.appendFormat("Permission Denial: "
            "can't dump SurfaceFlinger from pid=%d, uid=%d\n", callingPid, callingUid);
        write(fd, result.string(), result.size());
    } else {
        //dump klo
        if (isFeatureOn()) {
            const char *arg = NULL;
            Vector<const char *> *c_args = new Vector<const char *>();
            for (int i = 0; i < args.size(); i++) {
                c_args->add(strdup(String8(args.itemAt(i)).string()));
            }
            klo_dump(fd, c_args->size(), c_args->editArray());
            for (int i = 0; i < c_args->size(); i++) {
                free((char*)c_args->itemAt(i));
            }
            delete c_args;
        }
        else {
            result.appendFormat("KLO feature not enabled on the target.\n");
            write(fd, result.string(), result.size());
        }
    }
    return 0;
}

void SystemKloProxy::addDefaultMonitor()
{
    sp<KloEventThread> native_monitor = new KloEventThread("native_monitor", SOURCE_TOMESTONE);
    mEventsMonitor.add(SOURCE_TOMESTONE, native_monitor);
#ifdef OCTVM_USES_LOGD
    sp<KloEventThread> events_monitor = new KloEventThread("events_monitor", SOURCE_LOGD_LOGGER);
    mEventsMonitor.add(SOURCE_LOGD_LOGGER, events_monitor);
    ALOGD("add logd events monitor");
#else
    sp<KloEventThread> events_monitor = new KloEventThread("events_monitor", SOURCE_KERNEL_LOGGER);
    mEventsMonitor.add(SOURCE_KERNEL_LOGGER, events_monitor);
    ALOGD("add kernel logger events monitor");
#endif
}

void SystemKloProxy::startKloMonitor()
{
    //start event processing thread
    if (start_processing_thread() < 0) {
        ALOGE("start Klo processing thread failed! Please retry.");
        return;
    }
    //start event monitor
    for (int i = 0; i < mEventsMonitor.size(); i++) {
        sp<KloEventThread> monitor_thread = mEventsMonitor.valueAt(i);
        monitor_thread->run(monitor_thread->getName(), PRIORITY_NORMAL);
    }
}

void SystemKloProxy::stopKloMonitor()
{
    //stop event monitor
    for (int i = 0; i < mEventsMonitor.size(); i++) {
        sp<KloEventThread> monitor_thread = mEventsMonitor.valueAt(i);
        monitor_thread->requestExit();
    }
    //notify event processing thread to stop
    stop_processing_thread();
}

int32_t SystemKloProxy::FeatureEnable(bool enable) {
    if (isFeatureOn() == enable) return 0;
    int32_t ret = property_set(FeaturePropKey.string(), enable?"on":"off");
    if ( ret == 0) {
        if (enable)
            startKloMonitor();
        else
            stopKloMonitor();
    }
    else {
        ALOGE("FeatureEnable: failed to set feature property %s", enable?"on":"off");
    }
    return ret;
}

SystemKloProxy::KloEventThread::KloEventThread(const char* sourceName, int sourceType)
{
    mSourceName = String8(sourceName);
    mSourceType = sourceType;
}

bool SystemKloProxy::KloEventThread::threadLoop()
{
    bool resource_ready = false;
    ALOGD("%s start", mSourceName.string());
    //prepare resource
    while (!exitPending()) {
        if (klo_prepare_observer(mSourceType) == 0) {
            resource_ready = true;
            break;
        } else {
            //retry after sleep
            usleep(10000*1000);
        }
    }
    //main loop for polling events
    while (!exitPending()) {
        void *event = NULL;
        int eventType = klo_poll_events(&event, mSourceType);
        if (eventType > 0) {
            klo_notify_event(event, eventType);
        }
    }
    if (resource_ready) {
        klo_destroy_observer(mSourceType);
    }
    ALOGD("%s exit", mSourceName.string());
    return false;
}

}; // namespace android
