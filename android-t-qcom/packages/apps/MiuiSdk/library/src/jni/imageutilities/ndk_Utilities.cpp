/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#include <jni.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <android/log.h>
#include <android/bitmap.h>

#ifndef LOG_TAG
#define LOG_TAG "MiuiSdk"
#endif

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__)

#define NELEM(x)  (sizeof(x)/sizeof(x[0]))

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} COLOR;

inline int min(int a, int b) {
    return a > b ? b : a;
}

inline int max(int a, int b) {
    return a > b ? a : b;
}

static void fastBlur(JNIEnv* env, jobject, jobject jbitmapIn, jobject jbitmapOut, jint radius) {

    AndroidBitmapInfo bmpInInfo;
    AndroidBitmapInfo bmpOutInfo;

    memset(&bmpInInfo, 0, sizeof(bmpInInfo));
    memset(&bmpOutInfo, 0, sizeof(bmpOutInfo));

    AndroidBitmap_getInfo(env, jbitmapIn, &bmpInInfo);
    AndroidBitmap_getInfo(env, jbitmapOut, &bmpOutInfo);

    if (bmpInInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888 || bmpOutInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("only ARGB_8888 support.");
        return;
    }

    int h = bmpInInfo.height;
    int w = bmpInInfo.width;
    int inRealW = bmpInInfo.stride;
    int outRealW = bmpOutInfo.stride;

    void *pInPixels = NULL;
    void *pOutPixels = NULL;

    AndroidBitmap_lockPixels(env, jbitmapIn, &pInPixels);
    AndroidBitmap_lockPixels(env, jbitmapOut, &pOutPixels);

    COLOR* input = (COLOR*) pInPixels;
    COLOR* output = (COLOR*) pOutPixels;

    int wm = w - 1;
    int hm = h - 1;
    int wh = w * h;
    int whMax = max(w, h);
    int div = radius + radius + 1;

    int *r = new int[wh];
    int *g = new int[wh];
    int *b = new int[wh];
    int *a = new int[wh];
    int rsum, gsum, bsum, asum, x, y, i, yp, yi, yw;
    COLOR p;
    int vmin[whMax];

    int divsum = (div + 1) >> 1;
    divsum *= divsum;
    int* dv = new int[256 * divsum];
    int count = 256 * divsum;
    for (i = 0; i < count; i++) {
        dv[i] = (i / divsum);
    }

    yw = yi = 0;

    int stack[div][4];
    int stackpointer;
    int stackstart;
    int rbs;
    int ir;
    int ip;
    int r1 = radius + 1;
    int routsum, goutsum, boutsum, aoutsum;
    int rinsum, ginsum, binsum, ainsum;

    for (y = 0; y < h; y++) {
        rinsum = ginsum = binsum = ainsum = routsum = goutsum = boutsum = aoutsum = rsum = gsum = bsum = asum = 0;
        for (i = -radius; i <= radius; i++) {
            p = input[yw + min(wm, max(i, 0))];

            ir = i + radius; // same as sir

            stack[ir][0] = p.red;
            stack[ir][1] = p.green;
            stack[ir][2] = p.blue;
            stack[ir][3] = p.alpha;
            rbs = r1 - abs(i);
            rsum += stack[ir][0] * rbs;
            gsum += stack[ir][1] * rbs;
            bsum += stack[ir][2] * rbs;
            asum += stack[ir][3] * rbs;
            if (i > 0) {
                rinsum += stack[ir][0];
                ginsum += stack[ir][1];
                binsum += stack[ir][2];
                ainsum += stack[ir][3];
            } else {
                routsum += stack[ir][0];
                goutsum += stack[ir][1];
                boutsum += stack[ir][2];
                aoutsum += stack[ir][3];
            }
        }
        stackpointer = radius;

        for (x = 0; x < w; x++) {
            r[yi] = dv[rsum];
            g[yi] = dv[gsum];
            b[yi] = dv[bsum];
            a[yi] = dv[asum];

            rsum -= routsum;
            gsum -= goutsum;
            bsum -= boutsum;
            asum -= aoutsum;

            stackstart = stackpointer - radius + div;
            ir = stackstart % div; // same as sir

            routsum -= stack[ir][0];
            goutsum -= stack[ir][1];
            boutsum -= stack[ir][2];
            aoutsum -= stack[ir][3];

            if (y == 0) {
                vmin[x] = min(x + radius + 1, wm);
            }
            p = input[yw + vmin[x]];

            stack[ir][0] = p.red;
            stack[ir][1] = p.green;
            stack[ir][2] = p.blue;
            stack[ir][3] = p.alpha;

            rinsum += stack[ir][0];
            ginsum += stack[ir][1];
            binsum += stack[ir][2];
            ainsum += stack[ir][3];

            rsum += rinsum;
            gsum += ginsum;
            bsum += binsum;
            asum += ainsum;

            stackpointer = (stackpointer + 1) % div;
            ir = (stackpointer) % div; // same as sir

            routsum += stack[ir][0];
            goutsum += stack[ir][1];
            boutsum += stack[ir][2];
            aoutsum += stack[ir][3];

            rinsum -= stack[ir][0];
            ginsum -= stack[ir][1];
            binsum -= stack[ir][2];
            ainsum -= stack[ir][3];

            yi++;
        }
        yw += inRealW;
    }
    for (x = 0; x < w; x++) {
        rinsum = ginsum = binsum = ainsum = routsum = goutsum = boutsum = aoutsum = rsum = gsum = bsum = asum = 0;
        yp = -radius * w;
        for (i = -radius; i <= radius; i++) {
            yi = max(0, yp) + x;

            ir = i + radius; // same as sir

            stack[ir][0] = r[yi];
            stack[ir][1] = g[yi];
            stack[ir][2] = b[yi];
            stack[ir][3] = a[yi];

            rbs = r1 - abs(i);

            rsum += r[yi] * rbs;
            gsum += g[yi] * rbs;
            bsum += b[yi] * rbs;
            asum += a[yi] * rbs;

            if (i > 0) {
                rinsum += stack[ir][0];
                ginsum += stack[ir][1];
                binsum += stack[ir][2];
                ainsum += stack[ir][3];
            } else {
                routsum += stack[ir][0];
                goutsum += stack[ir][1];
                boutsum += stack[ir][2];
                aoutsum += stack[ir][3];
            }

            if (i < hm) {
                yp += w;
            }
        }
        yi = x;
        stackpointer = radius;
        for (y = 0; y < h; y++) {
            output[yi].red = dv[rsum];
            output[yi].green = dv[gsum];
            output[yi].blue = dv[bsum];
            output[yi].alpha = dv[asum];

            rsum -= routsum;
            gsum -= goutsum;
            bsum -= boutsum;
            asum -= aoutsum;

            stackstart = stackpointer - radius + div;
            ir = stackstart % div; // same as sir

            routsum -= stack[ir][0];
            goutsum -= stack[ir][1];
            boutsum -= stack[ir][2];
            aoutsum -= stack[ir][3];

            if (x == 0) vmin[y] = min(y + r1, hm) * w;
            ip = x + vmin[y];

            stack[ir][0] = r[ip];
            stack[ir][1] = g[ip];
            stack[ir][2] = b[ip];
            stack[ir][3] = a[ip];

            rinsum += stack[ir][0];
            ginsum += stack[ir][1];
            binsum += stack[ir][2];
            ainsum += stack[ir][3];

            rsum += rinsum;
            gsum += ginsum;
            bsum += binsum;
            asum += ainsum;

            stackpointer = (stackpointer + 1) % div;
            ir = stackpointer; // same as sir

            routsum += stack[ir][0];
            goutsum += stack[ir][1];
            boutsum += stack[ir][2];
            aoutsum += stack[ir][3];

            rinsum -= stack[ir][0];
            ginsum -= stack[ir][1];
            binsum -= stack[ir][2];
            ainsum -= stack[ir][3];

            yi += outRealW;
        }
    }

    AndroidBitmap_unlockPixels(env, jbitmapIn);
    AndroidBitmap_unlockPixels(env, jbitmapOut);

    delete r;
    delete g;
    delete b;
    delete a;
    delete[] dv;
}

///////////////////////////////////////////////////////////////////////////////
// JNI Initializaion
///////////////////////////////////////////////////////////////////////////////

static const char* classPathName = "miui/graphics/BitmapFactory";

static JNINativeMethod gMETHODS[] = {
    {"native_fastBlur", "(Landroid/graphics/Bitmap;Landroid/graphics/Bitmap;I)V", (void*)fastBlur }
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
    LOGV("RegisterNatives start for '%s'", className);
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
        gMETHODS, NELEM(gMETHODS))) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

/*
 * This is called by the VM when the shared library is first loaded.
 */
jint JNI_OnLoad(JavaVM* vm, void*)
{
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    jint result = -1;
    JNIEnv* env = NULL;

    LOGE("JNI_OnLoad");

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed");
        goto bail;
    }

    env = uenv.env;

    if (registerNatives(env) != JNI_TRUE) {
        LOGE("registerNatives failed");
        goto bail;
    }

    result = JNI_VERSION_1_6;
    LOGV("Load success");
bail:
    return result;
}
