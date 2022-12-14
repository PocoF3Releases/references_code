/* Stack Blur v1.0 from
 *http://www.quasimondo.com/StackBlurForCanvas/StackBlurDemo.html
 *Stack Blur Algorithm by Mario Klingemann <mario@quasimondo.com>
 */

#include <jni.h>
#include <stdio.h>
#include <android/log.h>

#ifdef USE_EXQU_MODE
#include <android_runtime/android_util_AssetManager.h>
#endif

#ifdef USE_OPENGL_RENDERER
#include <Caches.h>
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "libthemeutils"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__)

static void miui_content_res_ThemeNativeUtils_terminateAtlas() {
    #ifdef USE_OPENGL_RENDERER
        #ifdef DEFINE_ATLAS_GLOBAL_VAR
            #ifndef PLATFORM_SDK_VERSION_GREATER_25
                android::uirenderer::AssetAtlas::sDisableAtlas = true;
            #endif
        #else
            if (android::uirenderer::Caches::hasInstance()) {
                android::uirenderer::Caches::getInstance().assetAtlas.terminate();
            }
        #endif
    #endif
}

static jboolean miui_content_res_ThemeNativeUtils_isContainXXXhdpiResource(
        JNIEnv* env, jobject clazz, jobject asset) {
    #ifdef USE_EXQU_MODE
        android::AssetManager* am = android::assetManagerForJavaObject(env, asset);
        const android::ResTable& res = am->getResources();
        return res.isContainXXXhdpi();
    #endif
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// JNI Initializaion
///////////////////////////////////////////////////////////////////////////////

static const char* classPathName = "miui/content/res/ThemeNativeUtils";

static JNINativeMethod gMETHODS[] = {
    {"nTerminateAtlas", "()V", (void*) miui_content_res_ThemeNativeUtils_terminateAtlas },
    {"nIsContainXXXhdpiResource", "(Landroid/content/res/AssetManager;)Z",
            (void*) miui_content_res_ThemeNativeUtils_isContainXXXhdpiResource },
};

/*
 * This is called by the VM when the shared library is first loaded.
 */

typedef union {
    JNIEnv* env;
    void* venv;
} UnionJNIEnvToVoid;

/*
* Register several native methods for one class.
*/
static int registerNativeMethods(JNIEnv* env, const char* className,
    JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

/*
* Register native methods for all classes we know about.
*
* returns JNI_TRUE on success.
*/
static int registerNatives(JNIEnv* env)
{
    if (!registerNativeMethods(env, classPathName,
        gMETHODS, sizeof(gMETHODS) / sizeof(gMETHODS[0]))) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

/*
 * This is called by the VM when the shared library is first loaded.
 */
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    jint result = -1;
    JNIEnv* env = NULL;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed");
        goto fail;
    }

    env = uenv.env;

    if (registerNatives(env) != JNI_TRUE) {
        LOGE("registerNatives failed");
        goto fail;
    }

    result = JNI_VERSION_1_6;
    LOGV("Theme： libthemeutil.so load success");
fail:
    return result;
}
