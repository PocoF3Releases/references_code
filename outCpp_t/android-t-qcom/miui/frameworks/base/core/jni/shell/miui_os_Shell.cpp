#define LOG_TAG "ShellServiceJni"

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <android_runtime/AndroidRuntime.h>
#include "jni.h"
#include "IShellService.h"

#include <nativehelper/JNIHelp.h>

using namespace android;

static sp<IShellService> gShellService = NULL;
static Mutex gLock;
static const sp<IShellService>& getShellService()
{
    // We'd check whether the service is still alive.
    // If the service died for some reason, it would
    // re-register itself. Otherwise, we'll nerver get
    // the newly registered binder.

    // further, double-checked-locking without explicit
    // memory fences in C++ is not safe.
    // http://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11

    Mutex::Autolock _l(gLock);

    if (gShellService == NULL ||
#ifdef HAS_ARGUMENT_ASBINDER
        !IInterface::asBinder(gShellService)->isBinderAlive()) {
#else
        !gShellService->asBinder()->isBinderAlive()) {
#endif

        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
           binder = sm->getService(String16("miui.shell"));
           if (binder != NULL) break;
           ALOGE("Can't obtain ShellService");
           usleep(500000);
        } while (true);
        gShellService = interface_cast<IShellService>(binder);
    }

    return gShellService;
}

static jboolean android_miui_Shell_chmod(JNIEnv* env, jobject clazz, jstring path, jint mode, jint flag)
{
    return false;
}

static jboolean android_miui_Shell_chown(JNIEnv* env, jobject clazz, jstring path, jint owner, jint group, jint flag)
{
    return false;
}

static jboolean android_miui_Shell_write(JNIEnv* env, jobject clazz, jstring path, jstring buffer)
{
    return false;
}

static jboolean android_miui_Shell_copy(JNIEnv* env, jobject clazz, jstring source, jstring dest)
{
    return false;
}

static jboolean android_miui_Shell_link(JNIEnv* env, jobject clazz, jstring oldpath, jstring newpath)
{
    return false;
}

static jboolean android_miui_Shell_mkdirs(JNIEnv* env, jobject clazz, jstring path)
{
    return false;
}

static jboolean android_miui_Shell_move(JNIEnv* env, jobject clazz, jstring oldpath, jstring newpath)
{
    return false;
}

static jboolean android_miui_Shell_remove(JNIEnv* env, jobject clazz, jstring path)
{
    return false;
}

static jboolean android_miui_Shell_run(JNIEnv* env, jobject clazz, jstring cmd)
{
    return false;
}

static jboolean android_miui_Shell_runShell(JNIEnv* env, jobject clazz, jstring cmd)
{
    return false;
}

static jlong android_miui_Shell_getRuntimeSharedValue(JNIEnv* env, jobject clazz, jstring key)
{
    long ret;
    const char *keyStr = env->GetStringUTFChars(key, NULL);
    const sp<IShellService>& shell = getShellService();

    ret = shell->getRuntimeSharedValue(keyStr);

    env->ReleaseStringUTFChars(key, keyStr);

    return ret;
}

static jboolean android_miui_Shell_setRuntimeSharedValue(JNIEnv* env, jobject clazz, jstring key, jlong value)
{
    int err;
    const char *keyStr = env->GetStringUTFChars(key, NULL);
    const sp<IShellService>& shell = getShellService();

    err = shell->setRuntimeSharedValue(keyStr, value);

    env->ReleaseStringUTFChars(key, keyStr);

    return err == NO_ERROR;
}

static jboolean android_miui_Shell_boostCpuPulse(JNIEnv* env, jobject clazz)
{
    return false;
}

static jboolean android_miui_Shell_setfilecon(JNIEnv* env, jobject clazz, jstring path, jstring scontext)
{
    return false;
}

static jboolean android_miui_Shell_setProperty(JNIEnv* env, jobject clazz, jstring key, jstring value)
{
    return false;
}

static jboolean android_miui_Shell_writeByteArray(JNIEnv * env, jobject clazz, jstring path, jboolean append, jbyteArray bytes)
{
    return false;
}

static jbyteArray android_miui_Shell_readByteArray(JNIEnv * env, jobject clazz, jstring path)
{
    return NULL;
}

static const JNINativeMethod methods[] = {
    { "nativeChmod",                 "(Ljava/lang/String;II)Z",                 (void*)android_miui_Shell_chmod },
    { "nativeChown",                 "(Ljava/lang/String;III)Z",                (void*)android_miui_Shell_chown },
    { "nativeWrite",                 "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)android_miui_Shell_write },
    { "nativeCopy",                  "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)android_miui_Shell_copy },
    { "nativeLink",                  "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)android_miui_Shell_link },
    { "nativeMkdirs",                "(Ljava/lang/String;)Z",                   (void*)android_miui_Shell_mkdirs },
    { "nativeMove",                  "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)android_miui_Shell_move },
    { "nativeRemove",                "(Ljava/lang/String;)Z",                   (void*)android_miui_Shell_remove },
    { "nativeRun",                   "(Ljava/lang/String;)Z",                   (void*)android_miui_Shell_run },
    { "nativeRunShell",              "(Ljava/lang/String;)Z",                   (void*)android_miui_Shell_runShell },
    { "nativeGetRuntimeSharedValue", "(Ljava/lang/String;)J",                   (void*)android_miui_Shell_getRuntimeSharedValue },
    { "nativeSetRuntimeSharedValue", "(Ljava/lang/String;J)Z",                  (void*)android_miui_Shell_setRuntimeSharedValue },
    { "nativeBoostCpuPulse",         "()Z",                                     (void*)android_miui_Shell_boostCpuPulse },
    { "nativeSetfilecon",            "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)android_miui_Shell_setfilecon },
    { "nativeSetProperty",           "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)android_miui_Shell_setProperty },
    { "nativeWriteByteArray",        "(Ljava/lang/String;Z[B)Z",                (void*)android_miui_Shell_writeByteArray },
    { "nativeReadByteArray",         "(Ljava/lang/String;)[B",                  (void*)android_miui_Shell_readByteArray },
};

static const char* const kClassPathName = "android/miui/Shell";

jboolean registerNatives(JNIEnv* env)
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

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{

    jint result = -1;
    JNIEnv* env = NULL;

    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed");
        goto fail;
    }

    if (registerNatives(env) != JNI_TRUE) {
        goto fail;
    }

    result = JNI_VERSION_1_4;

fail:
    return result;
}
