
#include <utils/Log.h>
#include <utils/threads.h>
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/Log.h"
#include "android_os_Parcel.h"
#include "android_util_Binder.h"
#include <binder/Parcel.h>
#include "jni.h"
#include <nativehelper/JNIHelp.h>
#include "utils/Errors.h"
#include "DisplayFeature.h"

#define LOG_TAG "DisplayFeature-JNI"

namespace android {

struct fields_t {
    jfieldID context;
    jmethodID post_event;
};

static fields_t fields;
static Mutex sLock;

// ref-counted object for callbacks
class JNIDisplayFeatureListener: public DisplayFeatureListener
{
public:
    JNIDisplayFeatureListener(JNIEnv* env, jobject thiz, jobject weak_thiz);
    ~JNIDisplayFeatureListener();
    virtual void notify(int msg, int ext1, int ext2, const Parcel *obj = NULL);
private:
    JNIDisplayFeatureListener();
    jclass      mClass;     // Reference to DisplayFeature class
    jobject     mObject;    // Weak ref to DisplayFeature Java object to call on
};

JNIDisplayFeatureListener::JNIDisplayFeatureListener(JNIEnv* env, jobject thiz, jobject weak_thiz)
{
    // Hold onto the DisplayFeature class for use in calling the static method
    // that posts events to the application thread.
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        ALOGE("Can't find android/display/DisplayFeature");
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);

    // We use a weak reference so the DisplayFeature object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject  = env->NewGlobalRef(weak_thiz);
}

JNIDisplayFeatureListener::~JNIDisplayFeatureListener()
{
    // remove global references
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}

void JNIDisplayFeatureListener::notify(int msg, int ext1, int ext2, const Parcel *obj)
{
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    if (obj && obj->dataSize() > 0) {
        jobject jParcel = createJavaParcelObject(env);
        if (jParcel != NULL) {
            Parcel* nativeParcel = parcelForJavaObject(env, jParcel);
            nativeParcel->setData(obj->data(), obj->dataSize());
            env->CallStaticVoidMethod(mClass, fields.post_event, mObject,
                    msg, ext1, ext2, jParcel);
            env->DeleteLocalRef(jParcel);
        }
    } else {
        env->CallStaticVoidMethod(mClass, fields.post_event, mObject,
                msg, ext1, ext2, NULL);
    }
    if (env->ExceptionCheck()) {
        ALOGW("An exception occurred while notifying an event.");
        LOGW_EX(env);
        env->ExceptionClear();
    }
}

static sp<DisplayFeature> getDisplayFeature(JNIEnv* env, jobject thiz)
{
    Mutex::Autolock l(sLock);
    DisplayFeature* const p = (DisplayFeature*)env->GetLongField(thiz, fields.context);
    return sp<DisplayFeature>(p);
}

static sp<DisplayFeature> setDisplayFeature(JNIEnv* env, jobject thiz, const sp<DisplayFeature>& player)
{
    Mutex::Autolock l(sLock);
    sp<DisplayFeature> old = (DisplayFeature*)env->GetLongField(thiz, fields.context);
    if (player.get()) {
        player->incStrong((void*)setDisplayFeature);
    }
    if (old != 0) {
        old->decStrong((void*)setDisplayFeature);
    }
    env->SetLongField(thiz, fields.context, (jlong)player.get());
    return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_display_feature_call(JNIEnv *env, jobject thiz, status_t opStatus, const char* exception, const char *message)
{
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
        if (opStatus != (status_t) OK) {
            sp<DisplayFeature> df = getDisplayFeature(env, thiz);
            if (df != 0) df->notify(DISPLAY_ERROR, opStatus, 0);
        }
    } else {  // Throw exception!
        if ( opStatus == (status_t) INVALID_OPERATION ) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        } else if ( opStatus == (status_t) BAD_VALUE ) {
            jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        } else if ( opStatus == (status_t) PERMISSION_DENIED ) {
            jniThrowException(env, "java/lang/SecurityException", NULL);
        } else if ( opStatus != (status_t) OK ) {
            if (strlen(message) > 230) {
               // if the message is too long, don't bother displaying the status code
               jniThrowException( env, exception, message);
            } else {
               char msg[256];
                // append the status code to the message
               sprintf(msg, "%s: status=0x%X", message, opStatus);
               jniThrowException( env, exception, msg);
            }
        }
    }
}

static void android_display_DisplayFeature_init(JNIEnv *env)
{
    jclass clazz;

    clazz = env->FindClass("android/display/DisplayFeature");
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (fields.context == NULL) {
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        return;
    }
}

static void android_display_DisplayFeature_release(JNIEnv *env, jobject thiz)
{
    sp<DisplayFeature> df = setDisplayFeature(env, thiz, 0);
    if (df != NULL) {
        // this prevents native callbacks after the object is released
        df->setListener(0);
    }
}

static void android_display_DisplayFeature_setup(
        JNIEnv *env, jobject thiz, jobject weak_this)
{
    sp<DisplayFeature> df = NULL;

    bool bOk = false;

    sp<JNIDisplayFeatureListener> listener = new JNIDisplayFeatureListener(env, thiz, weak_this);

    if (!bOk){
        df = new DisplayFeature();
    }
    if (df == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }
    df->setListener(listener);

    // Stow our new C++ DisplayFeature in an opaque field in the Java object.
    setDisplayFeature(env, thiz, df);
}

static void android_display_DisplayFeature_setFeature(
    JNIEnv *env, jobject thiz, jint caseId, jint modeId, jint cookie)
{
    sp<DisplayFeature> df = getDisplayFeature(env, thiz);
    if (df == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_display_feature_call(env, thiz, df->setFeatureEnable(caseId, modeId, cookie), "java/io/IOException", "setFeature failed." );
}

static void android_display_DisplayFeature_setFunction(
    JNIEnv *env, jobject thiz, jint displayId, jint caseId, jint modeId, jint cookie)
{
    sp<DisplayFeature> df = getDisplayFeature(env, thiz);
    if (df == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_display_feature_call(env, thiz, df->setFunctionEnable(displayId, caseId, modeId, cookie), "java/io/IOException", "setFunction failed." );
}

static void android_display_DisplayFeature_setBrightness(
    JNIEnv *env, jobject thiz, jint modeId, jint cookie)
{
    sp<DisplayFeature> df = getDisplayFeature(env, thiz);
    if (df == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
}

static void android_display_DisplayFeature_finalize(
        JNIEnv *env, jobject thiz)
{
    sp<DisplayFeature> df = getDisplayFeature(env, thiz);
    if (df != NULL) {
        ALOGW("DisplayFeature finalized without being released");
    }
    android_display_DisplayFeature_release(env, thiz);
}

static const JNINativeMethod gMethods[] = {
    { "init_native", "()V", (void*)android_display_DisplayFeature_init },
    { "native_setup", "(Ljava/lang/Object;)V", (void *)android_display_DisplayFeature_setup },
    { "finalize_native", "()V", (void*)android_display_DisplayFeature_finalize },
    { "setFeature_native", "(III)V", (void*)android_display_DisplayFeature_setFeature },
    { "setFunction_native", "(IIII)V", (void*)android_display_DisplayFeature_setFunction },
    { "setBrightness_native", "(II)V", (void*)android_display_DisplayFeature_setBrightness },
};

int register_android_display_DisplayFeature(JNIEnv *env)
{
    return AndroidRuntime::registerNativeMethods(env,
                "android/display/DisplayFeature", gMethods, NELEM(gMethods));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved __unused)
{

    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_android_display_DisplayFeature(env) < 0) {
        ALOGE("ERROR: DisplayFeature native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

}
