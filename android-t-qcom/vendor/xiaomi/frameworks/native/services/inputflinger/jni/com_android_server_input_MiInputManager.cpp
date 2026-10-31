#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "InputConfig_JNI"
#include <utils/Log.h>
#include <jni.h>
#include <nativehelper/JNIHelp.h>
#include <binder/Parcel.h>
#include "MiInputManager.h"
#include <android_view_MotionEvent.h>
#include <android_runtime/AndroidRuntime.h>
#include <android_runtime/Log.h>
#include "MiInputManagerStubImpl.h"
#include "../reader/MiInputReaderStubImpl.h"

using namespace android;

static struct {
    jclass clazz;
} gMotionEventClassInfo;

static struct {
    jclass clazz;
    jmethodID notifyTouchMotionEvent;
} gMiInputManagerClassInfo;

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! (var), "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! (var), "Unable to find method " methodName);

class NativeMiInputManager : public virtual RefBase, public virtual MiuiInputMapperPolicyInterface {
private:
    jobject mMiInputManagerObj;
    sp<MiInputManager> mMiInputManager;

protected:
    virtual ~NativeMiInputManager();

public:
    NativeMiInputManager(jobject miInputManagerObj);
    inline sp<MiInputManager> getMiInputManager() const { return mMiInputManager; }
    virtual void notifyTouchMotionEvent(const MotionEvent* motionEvent) override;
    static bool checkAndClearExceptionFromCallback(JNIEnv* env, const char* methodName);
};

NativeMiInputManager::NativeMiInputManager(jobject miInputManagerObj) {
    MiInputManager* miInputManager = MiInputManager::getInstance();
    mMiInputManager = miInputManager;
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    mMiInputManagerObj = env->NewGlobalRef(miInputManagerObj);
    MiInputManagerStubImpl::getInstance()->getMiuiInputMapper()->setPolicy(this);
}

NativeMiInputManager::~NativeMiInputManager() {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    env->DeleteGlobalRef(mMiInputManagerObj);
}

bool NativeMiInputManager::checkAndClearExceptionFromCallback(JNIEnv* env, const char* methodName) {
    if (env->ExceptionCheck()) {
        ALOGE("An exception was thrown by callback '%s'.", methodName);
        LOGE_EX(env);
        env->ExceptionClear();
        return true;
    }
    return false;
}

void NativeMiInputManager::notifyTouchMotionEvent(const MotionEvent *motionEvent) {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    jobject motionEventObj = android_view_MotionEvent_obtainAsCopy(env, motionEvent);
    if (!motionEventObj){
        ALOGE("Failed to obtain motion event object for notifyTouchMotionEvent.");
        return;
    }
    env->CallVoidMethod(mMiInputManagerObj, gMiInputManagerClassInfo.notifyTouchMotionEvent,
                        motionEventObj);
    checkAndClearExceptionFromCallback(env,"notifyTouchMotionEvent");
    env->DeleteLocalRef(motionEventObj);
}

static jlong nativeInit(JNIEnv* env, jclass clazz, jobject miInputManagerObj) {
    NativeMiInputManager* nativeMiInputManager = new NativeMiInputManager(miInputManagerObj);
    nativeMiInputManager->incStrong(0);
    return reinterpret_cast<jlong>(nativeMiInputManager);
}

static void nativeSetInputConfig(JNIEnv* env, jclass /* clazz */, jlong ptr,
    jint configType, jlong parcelPtr) {
    Parcel* parcel = reinterpret_cast<Parcel*>(parcelPtr);
    NativeMiInputManager* nativeMiInputManager = reinterpret_cast<NativeMiInputManager*>(ptr);
    if(nativeMiInputManager  == nullptr || parcel ==nullptr){
        ALOGE("input nativeSetInputConfig  error nativeMiInputManager null:%d, parcelPtr null:%d",
              (nativeMiInputManager == nullptr), (parcel == nullptr));
        return;
    }
    nativeMiInputManager->getMiInputManager()->setInputConfig(configType, parcel);
}

static void nativeInjectMotionEvent(JNIEnv* env, jclass /* clazz */,
                                    jlong ptr, jobject motionEvent, jint mode) {
    NativeMiInputManager* nativeMiInputManager = reinterpret_cast<NativeMiInputManager*>(ptr);

    if (env->IsInstanceOf(motionEvent, gMotionEventClassInfo.clazz)) {
        const MotionEvent* event = android_view_MotionEvent_getNativePtr(env, motionEvent);
        if (!event) {
            jniThrowRuntimeException(env, "Could not read contents of MotionEvent object.");
            return ;
        }
        nativeMiInputManager->getMiInputManager()->injectMotionEvent(event, mode);
    } else {
        jniThrowRuntimeException(env, "Invalid input event type.");
    }
}

static jboolean nativeSetCursorPosition(JNIEnv* env, jclass /* clazz */, jlong ptr, jfloat x,
                                        jfloat y) {
    return MiInputReaderStubImpl::getInstance()->setCursorPosition(x, y);
}

static jboolean nativeHideCursor(JNIEnv* env, jclass /* clazz */, jlong ptr) {
    return MiInputReaderStubImpl::getInstance()->hideCursor();
}

static jstring nativeDump (JNIEnv* env, jobject nativeImplObjr) {
    std::string dumpStr = "";
    MiInputManagerStubImpl *miInputManagerStubImpl = MiInputManagerStubImpl::getInstance();
    miInputManagerStubImpl->dump(dumpStr);
    return env->NewStringUTF(dumpStr.c_str());
}

static const JNINativeMethod methods[] = {
        {"nativeInit",
         "(Lcom/android/server/input/MiInputManager;)J",
         (void*)nativeInit},
        {"nativeSetInputConfig", "(JIJ)V", (void*) nativeSetInputConfig},
        {"nativeInjectMotionEvent", "(JLandroid/view/MotionEvent;I)V",
         (void*)nativeInjectMotionEvent},
        {"nativeDump", "()Ljava/lang/String;", (void*)nativeDump},
        {"nativeSetCursorPosition", "(JFF)Z", (void*)nativeSetCursorPosition},
        {"nativeHideCursor", "(J)Z", (void*)nativeHideCursor},
};

static int registerNativeMethods(JNIEnv* env, const char* className,
    const JNINativeMethod* gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'", className);
        return JNI_ERR;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        ALOGE("RegisterNatives failed for '%s'", className);
        return JNI_ERR;
    }
    return JNI_OK;
}

int register_com_android_server_input_MiInputManager(JNIEnv *env) {
    FIND_CLASS(gMotionEventClassInfo.clazz, "android/view/MotionEvent");
    gMotionEventClassInfo.clazz = jclass(env->NewGlobalRef(gMotionEventClassInfo.clazz));

    FIND_CLASS(gMiInputManagerClassInfo.clazz, "com/android/server/input/MiInputManager");
    gMiInputManagerClassInfo.clazz = reinterpret_cast<jclass>(
      env->NewGlobalRef(gMiInputManagerClassInfo.clazz));
    GET_METHOD_ID(gMiInputManagerClassInfo.notifyTouchMotionEvent, gMiInputManagerClassInfo.clazz,
                  "notifyTouchMotionEvent", "(Landroid/view/MotionEvent;)V");

    return registerNativeMethods(env, "com/android/server/input/MiInputManager",
                                 methods, NELEM(methods));
}

