#include <jni.h>
#include <android/log.h>
#include "BlurDrawableFunctor.h"

#define MYLOG(...)  __android_log_print(ANDROID_LOG_ERROR,"BLUR_DEBUG",__VA_ARGS__)

using namespace android;
extern "C"
JNIEXPORT jlong JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nCreateNativeFunctor(JNIEnv *env, jclass clazz,jint mBlurWidth, jint mBlurHeight) {
    MYLOG("nCreateNativeFunctor");
    return  (jlong)new BlurDrawableFunctor();
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nDeleteNativeFunctor(JNIEnv *env, jclass clazz,
                                                              jlong m_functor) {
    MYLOG("nDeleteNativeFunctor");
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *)m_functor;
    delete drawFunctor;
    drawFunctor = nullptr;
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nEnableBlur(JNIEnv *env, jclass clazz, jlong m_functor,
                                                     jboolean enable) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *) m_functor;
    drawFunctor->mFunctor.isEnable = enable;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nSetBlurCornerRadii(JNIEnv *env, jclass clazz,
                                                             jlong m_functor, jfloatArray radio) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *)m_functor;
    if (env->GetArrayLength(radio) != 4) {
        return;
    }

    jfloat* input = env->GetFloatArrayElements(radio, NULL);
    for (int i = 0;i<4;i++) {
        drawFunctor->mFunctor.mBlurCornerRadii[i] = input[i];
    }
    env->ReleaseFloatArrayElements( radio, input, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nSetBlurRatio(JNIEnv *env, jclass clazz, jlong m_functor,
                                                       jfloat ratio) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *) m_functor;
    drawFunctor->mFunctor.mBlurRatio = ratio;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nSetMixColor(JNIEnv *env, jclass clazz, jlong m_functor,
                                                      jint color, jint mode) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *) m_functor;
    drawFunctor->mFunctor.mMixColor.clear();
    drawFunctor->mFunctor.mMixMode.clear();
    drawFunctor->mFunctor.mMixColor.push_back(color);
    drawFunctor->mFunctor.mMixMode.push_back(mode);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nAddMixColor(JNIEnv *env, jclass clazz, jlong m_functor,
                                                          jint color, jint mode) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *) m_functor;
    drawFunctor->mFunctor.mMixColor.push_back(color);
    drawFunctor->mFunctor.mMixMode.push_back(mode);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nClearMixColor(JNIEnv *env, jclass clazz, jlong m_functor) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *) m_functor;
    if (drawFunctor->mFunctor.mMixColor.size() > 0) {
        drawFunctor->mFunctor.mMixColor.clear();
        drawFunctor->mFunctor.mMixMode.clear();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nSetBlurMode(JNIEnv *env, jclass clazz, jlong m_functor,
                                                      jint mode) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *) m_functor;
    drawFunctor->mFunctor.mBlurMode = mode;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nNeedUpdateBounds(JNIEnv *env, jclass type, jlong m_functor,
                                                               jboolean needUpdate) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *) m_functor;
    drawFunctor->mFunctor.mNeedUpdateBounds = needUpdate;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_miui_blur_drawable_BlurDrawable_nSetAlpha(JNIEnv *env, jclass type, jlong mFunctor,
                                                       jfloat alpha) {
    BlurDrawableFunctor *drawFunctor = (BlurDrawableFunctor *) mFunctor;
    drawFunctor->mFunctor.mAlpha = alpha;
}
