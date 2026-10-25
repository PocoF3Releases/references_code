#ifndef LOG_TAG
#define LOG_TAG "jni.syspressure"
#endif

#include <utils/Log.h>
#include <stdio.h>
#include <string.h>
#include <linux/netlink.h>
#include <android-base/stringprintf.h>
#include "jni.h"
#include "core_jni_helpers.h"

#ifdef MI_SYS_PRESSURE_CRL_H
#include "SysPressureCtrlManager.h"
#endif

namespace android {
static constexpr const char* const kServicePathName =
                    "com/android/server/am/SystemPressureController";

class JNISystemPressureController {
public:
    JNISystemPressureController(jobject obj, jmethodID cpuPressureMid,
            jmethodID cpuExceptionMid, jmethodID thermalTempChangeMID) :
                    mJSysPressureCtrlObj(obj),
                    mJNotifyCpuPressureMID(cpuPressureMid),
                    mJNotifyCpuExceptionMID(cpuExceptionMid),
                    mJNotifyThermalTempMID(thermalTempChangeMID) {

    }

    ~JNISystemPressureController() {
        bool needsDetach = false;
        JNIEnv* env = getJNIEnv(needsDetach, false);
        if (env != NULL) {
            env->DeleteGlobalRef(mJSysPressureCtrlObj);
        } else {
            ALOG(LOG_ERROR, LOG_TAG, "leaking JNI object references");
        }
        if (needsDetach) {
            detachJNI();
        }
    }

    void notifyCpuPressureEvents(int level) {
        callVoidMethod(mJNotifyCpuPressureMID, level);
    }

    void notifyCpuExceptionEvents(int type) {
        callVoidMethod(mJNotifyCpuExceptionMID, type);
    }

    void notifyThermalTempChange(int temp) {
        callVoidMethod(mJNotifyThermalTempMID, temp);
    }

private:
    JNIEnv* getJNIEnv(bool& needsDetach, bool isAttach = true) {
        needsDetach = false;
        JNIEnv* env = nullptr;
        JavaVM* vm = AndroidRuntime::getJavaVM();
        if(!isAttach &&
                vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
            return env;
        }
        JavaVMAttachArgs args = { JNI_VERSION_1_4, "SystemPressureController", NULL };
        int result = vm->AttachCurrentThread(&env, (void*) &args);
        if (result != JNI_OK) {
            ALOG(LOG_ERROR, LOG_TAG, "thread attach failed: %#x", result);
            return NULL;
        }
        needsDetach = true;
        return env;
    }

    void detachJNI() {
        JavaVM* vm = AndroidRuntime::getJavaVM();
        int result = vm->DetachCurrentThread();
        if (result != JNI_OK) {
            ALOG(LOG_ERROR, LOG_TAG, "thread detach failed: %#x", result);
        }
    }

    void callVoidMethod(jmethodID methodId, int param) {
        bool needsDetach = false;
        JNIEnv* env = getJNIEnv(needsDetach);
        if (!env || !mJSysPressureCtrlObj || !methodId) {
            ALOG(LOG_ERROR, LOG_TAG, "env or mJSysPressureCtrlObj or method is null.");
            return;
        }
        env->CallVoidMethod(mJSysPressureCtrlObj, methodId, param);
        checkAndClearExceptionFromCallback(env, __FUNCTION__);
        if(needsDetach) detachJNI();
    }

    void checkAndClearExceptionFromCallback(JNIEnv* env, const char* methodName) {
        if (env->ExceptionCheck()) {
            ALOG(LOG_ERROR, LOG_TAG, "An exception was thrown by callback '%s'.", methodName);
            env->ExceptionClear();
        }
    }

private:
    jobject mJSysPressureCtrlObj;
    jmethodID mJNotifyCpuPressureMID;
    jmethodID mJNotifyCpuExceptionMID;
    jmethodID mJNotifyThermalTempMID;
};

std::shared_ptr<JNISystemPressureController> gJNISystemPressureController;

static void nInit(JNIEnv *env, jobject obj) {
#ifdef MI_SYS_PRESSURE_CRL_H
    jclass clazz = FindClassOrDie(env, kServicePathName);
    jmethodID cpuPressureMID = GetMethodIDOrDie(env, clazz, "notifyCpuPressureEvents", "(I)V");
    jmethodID cpuExceptionMID = GetMethodIDOrDie(env, clazz, "notifyCpuExceptionEvents", "(I)V");
    jmethodID thermalTempMID = GetMethodIDOrDie(env, clazz, "notifyThermalTempChange", "(I)V");
    jobject sysPressureCtrlObj = MakeGlobalRefOrDie(env, obj);
    gJNISystemPressureController = std::make_shared<JNISystemPressureController>(sysPressureCtrlObj,
                    cpuPressureMID, cpuExceptionMID, thermalTempMID);

    syspressure::SysPressureCtrlManager::getInstance()->init();
    syspressure::SysPressureCtrlManager::getInstance()->setCpuPressureCall(
            std::bind(&JNISystemPressureController::notifyCpuPressureEvents,
                    gJNISystemPressureController, std::placeholders::_1));

    syspressure::SysPressureCtrlManager::getInstance()->setCpuExceptionCall(
                std::bind(&JNISystemPressureController::notifyCpuExceptionEvents,
                        gJNISystemPressureController, std::placeholders::_1));

    syspressure::SysPressureCtrlManager::getInstance()->setThermalTempCall(
                std::bind(&JNISystemPressureController::notifyThermalTempChange,
                        gJNISystemPressureController, std::placeholders::_1));
#endif
}

static void nStartPressureMonitor(JNIEnv *env, jobject obj) {
#ifdef MI_SYS_PRESSURE_CRL_H
    syspressure::SysPressureCtrlManager::getInstance()->startPressureMonitor();
#endif
}

static void nEndPressureMonitor(JNIEnv *env, jobject obj) {
#ifdef MI_SYS_PRESSURE_CRL_H
    syspressure::SysPressureCtrlManager::getInstance()->endPressureMonitor();
#endif
}

static jlong nGetCpuUsage(JNIEnv *env, jobject, jint jpid, jstring jProcessName) {
#ifdef MI_SYS_PRESSURE_CRL_H
    const char * processName = env->GetStringUTFChars(jProcessName, 0);
    return syspressure::SysPressureCtrlManager::getInstance()->getCpuUsage(jpid, processName);
#else
    return 0;
#endif
}

static void nDumpCpuLimit(JNIEnv *env, jclass cls, jobject jPrintWriter) {
#ifdef MI_SYS_PRESSURE_CRL_H
    jclass printWriterCls = env->GetObjectClass(jPrintWriter);
    if(printWriterCls == NULL) {
        return;
    }
    jmethodID printlnMethod = env->GetMethodID(printWriterCls, "println", "(Ljava/lang/String;)V");
    if(printlnMethod == NULL) {
        return;
    }
    jstring dumContent = env->NewStringUTF("dump cpulimit current:");
    env->CallObjectMethod(jPrintWriter, printlnMethod, dumContent);
    env->DeleteLocalRef(dumContent);
#endif
}

static jstring nGetBackgroundCpuPolicy(JNIEnv *env, jclass cls) {
#ifdef MI_SYS_PRESSURE_CRL_H
    std::string cpupPolicy =
            syspressure::SysPressureCtrlManager::getInstance()->getBackgroundCpuPolicy();
    return env->NewStringUTF(cpupPolicy.c_str());
#else
    return env->NewStringUTF("0-3");
#endif
}
// ----------------------------------------------------------------------------

static const JNINativeMethod g_methods[] = {
        {"nInit", "()V",
                (void *) nInit},
        {"nStartPressureMonitor", "()V",
                        (void *) nStartPressureMonitor},
        {"nEndPressureMonitor", "()V",
                        (void *) nEndPressureMonitor},
        {"nGetCpuUsage", "(ILjava/lang/String;)J",
                    (void *) nGetCpuUsage},
        {"nDumpCpuLimit", "(Ljava/io/PrintWriter;)V",
                    (void *) nDumpCpuLimit},
        {"nGetBackgroundCpuPolicy", "()Ljava/lang/String;",
                            (void *) nGetBackgroundCpuPolicy},
};

int register_com_android_server_am_SystemPressureController(JNIEnv* env) {
    ALOG(LOG_DEBUG, LOG_TAG, "register_com_android_server_am_SystemPressureController");
    return RegisterMethodsOrDie(env, kServicePathName, g_methods, NELEM(g_methods));
}

}// end android
