#define LOG_TAG "PowerKeeper-JNI"

#include <limits.h>
#include <android_runtime/AndroidRuntime.h>

#include <utils/Looper.h>
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <utils/StrongPointer.h>
#include <cstring>

#include "powerkeeper_register.h"
#include "android_helper.h"

extern "C" {
#include "klo_internal.h"
}

using namespace android;

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

#define GET_FIELD_ID(var, clazz, fieldName, fieldDescriptor) \
        var = env->GetFieldID(clazz, fieldName, fieldDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find field " fieldName);

static struct {
    jmethodID sendEventLog;
    jmethodID filterOut;
} gServiceClassInfo;

class NativePowerKeeper : public virtual RefBase,
    public Thread{
protected:
    virtual ~NativePowerKeeper();
public:
    NativePowerKeeper(jobject serviceObj);
    status_t start();
    void processEvent(const unsigned char *buffer, size_t bufferSize);
    void notify();

    //Thread
    virtual bool threadLoop();

    bool mResourceReady;
private:

    bool checkAndClearExceptionFromCallback(JNIEnv* env, const char* methodName);

    int mSourceType;
    jobject mServiceObj;
    struct timeval mStartTime;
    bool mStart;
};


NativePowerKeeper::NativePowerKeeper(jobject serviceObj) {
    mStart = false;
    gettimeofday(&mStartTime, 0);
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    mServiceObj = env->NewGlobalRef(serviceObj);
    mResourceReady = false;
#ifdef OCTVM_USES_LOGD
    mSourceType = SOURCE_LOGD_LOGGER;
#else
    mSourceType = SOURCE_KERNEL_LOGGER;
#endif
    while (!exitPending()) {
        if (klo_prepare_observer(mSourceType) == 0) {
            mResourceReady = true;
            break;
        } else {
            usleep(1000*1000);
        }
    }
}

NativePowerKeeper::~NativePowerKeeper() {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    env->DeleteGlobalRef(mServiceObj);

    if (mResourceReady) {
#ifdef OCTVM_USES_LOGD
        klo_destroy_observer(SOURCE_LOGD_LOGGER);
#else
        klo_destroy_observer(SOURCE_KERNEL_LOGGER);
#endif
    }
}

bool NativePowerKeeper::threadLoop()
{
    void *event = NULL;
    int eventType = klo_poll_events(&event, mSourceType);

    if (eventType > 0) {
        if (event == NULL)
            return true;
        this->processEvent((unsigned char *)event, sizeof(*(struct stdlog_event *)event));
    }

    klo_do_free(eventType, event);

    return true;
}

status_t NativePowerKeeper::start() {
    status_t result = this->run("NativePowerKeeperEventReader", PRIORITY_URGENT_DISPLAY);
    bool resource_ready = this->mResourceReady;

    if (result) {
        ALOGE("Could not start EventReaderThread due result %d", result);
    }
    return result;
}

extern "C" jbyte* jniGetNonMovableArrayElements(C_JNIEnv* env, jarray arrayObj);

void NativePowerKeeper::processEvent(const unsigned char *buffer, size_t bufferSize) {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    struct stdlog_event *entry = (struct stdlog_event *)buffer;
    jboolean ret = env->CallBooleanMethod(mServiceObj, gServiceClassInfo.filterOut, entry->pid, entry->tag_id);
    //filter out useless log
    if (ret) {
        return ;
    }
    int len = strlen(entry->msg_body);
    if (entry->msg_body[len-1] == ']')
        entry->msg_body[len-1] = '\0';
    //the reason of "+SKIP" here is to skip '['
    int SKIP = ((entry->msg_body)[0] == '[') ? 1 : 0;
    //ALOGE("%s", (entry->msg_body)+SKIP);
    jstring msg = env->NewStringUTF((entry->msg_body)+SKIP);
    env->CallVoidMethod(mServiceObj, gServiceClassInfo.sendEventLog, msg, entry->tag_id);
    env->DeleteLocalRef(msg);
}

void NativePowerKeeper::notify() {
// TODO
    ALOGE("notify");
}

bool NativePowerKeeper::checkAndClearExceptionFromCallback(JNIEnv* env, const char* methodName) {
    if (env->ExceptionCheck()) {
        ALOGE("An exception was thrown by callback '%s'.", methodName);
        //ALOGE(env);
        env->ExceptionClear();
        return true;
    }
    return false;
}

static jlong com_miui_powerkeeper_EventLogManager_init(JNIEnv *env, jobject clazz,  jobject serviceObj) {
    NativePowerKeeper *ws = new NativePowerKeeper(serviceObj);
    ws->incStrong(serviceObj);
    return reinterpret_cast<jlong>(ws);
}

static void com_miui_powerkeeper_EventLogManager_start(JNIEnv *env, jobject clazz, jlong ptr) {
    NativePowerKeeper *ws =  reinterpret_cast<NativePowerKeeper*>(ptr);
    status_t result = ws->start();
    if (result) {
        jniThrowRuntimeException(env, "powerkeeper event manager could not be started.");
   }
}

static void com_miui_powerkeeper_EventLogManager_finalize(JNIEnv *env, jobject clazz, jobject serviceObj, jlong ptr) {
    if (ptr == 0) return;
    NativePowerKeeper *ws =  reinterpret_cast<NativePowerKeeper*>(ptr);
    ws->decStrong(serviceObj);
}


static const JNINativeMethod methods[] = {
    {"nativeInit",     "(Lcom/miui/powerkeeper/event/EventLogManager;)J",
        (void*)com_miui_powerkeeper_EventLogManager_init},
    {"nativeStart",    "(J)V",
        (void*)com_miui_powerkeeper_EventLogManager_start},
    {"nativeFinalize", "(Lcom/miui/powerkeeper/event/EventLogManager;J)V",
        (void*)com_miui_powerkeeper_EventLogManager_finalize}

};

static const char* const kClassPathName = "com/miui/powerkeeper/event/EventLogManager";

int  register_com_miui_powerkeeper_EventLogManager(JNIEnv* env)
{
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class EventLogManager");

    if (env->RegisterNatives(clazz, methods, NELEM(methods)) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return -1;
    }

    GET_METHOD_ID(gServiceClassInfo.sendEventLog, clazz
        , "sendEventLog", "(Ljava/lang/String;I)V");
    GET_METHOD_ID(gServiceClassInfo.filterOut, clazz
            , "filterOut", "(II)Z");
    return 0;
}

static int register_jni_procs(const RegJNIRec array[], size_t count, JNIEnv* env)
{
    for (size_t i = 0; i < count; i++) {
        if (array[i].mProc(env) < 0) {
            return -1;
        }
    }
    return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{

    jint result = -1;
    JNIEnv* env = NULL;

    ALOGE("on Load native EventLogManager");
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed");
        goto fail;
    }

    if (register_jni_procs(gRegJNI, NELEM(gRegJNI), env) < 0) {
        goto fail;
    }
    result = JNI_VERSION_1_4;

fail:

    return result;
}
