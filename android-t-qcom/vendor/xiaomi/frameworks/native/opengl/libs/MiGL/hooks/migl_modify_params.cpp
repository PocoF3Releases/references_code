#include "MiGLSettings.h"
#include "migl_modify_params.h"
#include <vector>
#include <string>
#include <sys/syscall.h>
#include <cutils/properties.h>
#include <cutils/log.h>
#include <sstream>
#include <fstream>
#include "SceneRecognizer.h"
#include <cmath>

using namespace android::migl;

namespace android {

// Define the various parameters that will control the shading rate.
#ifndef GL_QCOM_shading_rate
#define GL_QCOM_shading_rate 1
#define GL_SHADING_RATE_1X1_PIXELS_QCOM            0x96A6
#define GL_SHADING_RATE_1X2_PIXELS_QCOM            0x96A7
#define GL_SHADING_RATE_2X1_PIXELS_QCOM            0x96A8
#define GL_SHADING_RATE_2X2_PIXELS_QCOM            0x96A9
#define GL_SHADING_RATE_4X2_PIXELS_QCOM            0x96AC
#define GL_SHADING_RATE_4X4_PIXELS_QCOM            0x96AE
typedef void (GL_APIENTRYP PFNGLSHADINGRATEQCOM) (GLenum rate);
PFNGLSHADINGRATEQCOM glShadingRateQCOM;
#endif /* GL_QCOM_shading_rate */

bool mEnableScaleResolutionSize = false;
GLsizei mTargetTexWidthYuanshen = -1;
GLsizei mTargetTexHeightYuanshen = -1;
bool mIsYuanshenPostprocessLow = false;

bool mEnableShadingRate = false;
GLsizei mShadingRate = 0;

bool mEnableAFModify = false;
GLsizei mTargetAFValue = -1;

bool mEnableYSSceneRecognize = false;
sp<SceneRecognizer> mSceneRecognizer = nullptr;

bool migldebug = false;

void initGameSetting() {
    char value[PROPERTY_VALUE_MAX];
    property_get("vendor.migl.debug", value, "false");
    if (!strcmp(value, "true")) {
        migldebug = true;
    }

    glShadingRateQCOM = (PFNGLSHADINGRATEQCOM)((void*)eglGetProcAddress("glShadingRateQCOM"));
    if (glShadingRateQCOM == NULL) {
        ALOGE("libmigl:This GPU version is note support Variable Shading Rate");
    } else {
        if(migldebug) ALOGE("libmigl:Get the glShadingRateQCOM success");
    }

    mEnableScaleResolutionSize = MiGLSettings::getInstance().scaleResolutionSize();
    mTargetTexWidthYuanshen = MiGLSettings::getInstance().targetWidthSize();
    mTargetTexHeightYuanshen = MiGLSettings::getInstance().targetHeightSize();

    mEnableShadingRate = MiGLSettings::getInstance().modifyShadingRate();
    mShadingRate = MiGLSettings::getInstance().targetShadingRate();

    mEnableAFModify = MiGLSettings::getInstance().modifyAF();
    mTargetAFValue = MiGLSettings::getInstance().targetAFValue();

    mEnableYSSceneRecognize = MiGLSettings::getInstance().isEnableYSSceneRecognize();
    if (mEnableYSSceneRecognize && mSceneRecognizer == nullptr) {
        mSceneRecognizer = new SceneRecognizer();
        mSceneRecognizer->startThread();
    }

    if (migldebug) {
        ALOGE("migldebug: ScaleResolutionSize(%d %d %d), ShadingRate(%d %d),AFModify(%d %d), CachedValue= %d, YSSceneRecognize=%d",
            mEnableScaleResolutionSize, mTargetTexWidthYuanshen, mTargetTexHeightYuanshen, mEnableShadingRate, mShadingRate,
            mEnableAFModify, mTargetAFValue, MiGLSettings::getInstance().cacheGLValues(), mEnableYSSceneRecognize);
    }
}

void modifyRenderPrecisionYuanshen(GLsizei& width, GLsizei& height, int api) {
    if (!mEnableScaleResolutionSize) return;

    // L2 1080*2400
    if (!mIsYuanshenPostprocessLow && (
        (width == 1296 && height == 582)
        || (width == 1152 && height == 518)
        || (width == 1024 && height == 460)
        || (width == 864 && height == 388)
        || (width == 777 && height == 348))) {
            ALOGD("yunshen postprocess low");
            mIsYuanshenPostprocessLow = true;
    }
    // 1080*2400 downscale
    if (!mIsYuanshenPostprocessLow && (width > mTargetTexWidthYuanshen) &&
        ((width == 1620 && height == 728)
        || (width == 1440 && height == 648)
        || (width == 1280 && height == 576)
        || (width == 1080 && height == 486)
        || (width == 972 && height == 436))) {
        if (migldebug && api == 1) ALOGE("migldebug:modifyRenderPrecisionYuanshen, rf: %d*%d -> %d*%d",
                width, height, mTargetTexWidthYuanshen, mTargetTexHeightYuanshen);
        width = mTargetTexWidthYuanshen;
        height = mTargetTexHeightYuanshen;
    }
    // 1080*2400 upscale
    if (!mIsYuanshenPostprocessLow && (width < mTargetTexWidthYuanshen) &&
        ((width == 1620 && height == 728)
        || (width == 1440 && height == 648))) {
        if (migldebug && api == 1) ALOGE("migldebug:modifyRenderPrecisionYuanshen, rf: %d*%d -> %d*%d",
                width, height, mTargetTexWidthYuanshen, mTargetTexHeightYuanshen);
        width = mTargetTexWidthYuanshen;
        height = mTargetTexHeightYuanshen;
    }

    // L18
    // 1080*2520
    if (!mIsYuanshenPostprocessLow && (
        (width == 1296 && height == 555)
        || (width == 1024 && height == 438)
        || (width == 864 && height == 369)
        || (width == 777 && height == 332))) {
            ALOGD("yunshen postprocess low");
            mIsYuanshenPostprocessLow = true;
        }
    if (!mIsYuanshenPostprocessLow && (width > mTargetTexWidthYuanshen) &&
        ((width == 1620 && height == 694)
        || (width == 1280 && height == 548)
        || (width == 1080 && height == 462)
        || (width == 972 && height == 416))) {
        if (migldebug && api == 1) ALOGE("migldebug:modifyRenderPrecisionYuanshen, rf: %d*%d -> %d*%d",
                width, height, mTargetTexWidthYuanshen, mTargetTexHeightYuanshen);
        width = mTargetTexWidthYuanshen;
        height = mTargetTexHeightYuanshen;
    }
}

void modifyRenderPrecisionYuanshenShaderUniform(GLsizei count, GLfloat *value) {
    if (!mEnableScaleResolutionSize) return;
    if (count != 1) return;
    if (value[0] > 2 && value[1] > 2 && value[2] > 1 && value[2] < 2 && value[3] > 1 && value[3] < 2) {
        GLfloat width = value[0];
        GLfloat height = value[1];
        GLfloat _width = 1 + 1 / width;
        GLfloat _height = 1 + 1 / height;
        if (abs(value[2] - _width) < 0.0001 && abs(value[3] - _height) < 0.0001) {
            GLsizei widthInt = (GLsizei)width;
            GLsizei heightInt = (GLsizei)height;
            modifyRenderPrecisionYuanshen(widthInt, heightInt, 3);
            if (widthInt != value[0] && heightInt != value[1]) {
                width = (GLfloat)widthInt;
                height = (GLfloat)heightInt;
                _width = 1 + 1 / width;
                _height = 1 + 1 / height;
                if (migldebug) ALOGE("migldebug:modifyRenderPrecisionYuanshenShaderUniform, su: [%f, %f, %f, %f] -> [%f, %f, %f, %f]",
                    value[0], value[1], value[2], value[3], width, height, _width, _height);
                value[0] = width;
                value[1] = height;
                value[2] = _width;
                value[3] = _height;
            }
        }
    } else if (value[2] > 2 && value[3] > 2 && value[0] > 0 && value[0] < 1 && value[1] > 0 && value[1] < 1) {
        GLfloat width = value[2];
        GLfloat height = value[3];
        GLfloat _width = 1 / width;
        GLfloat _height = 1 / height;
        if (abs(value[0] - _width) < 0.0001 && abs(value[1] - _height) < 0.0001) {
            GLsizei widthInt = (GLsizei)width;
            GLsizei heightInt = (GLsizei)height;
            modifyRenderPrecisionYuanshen(widthInt, heightInt, 3);
            if (widthInt != value[2] && heightInt != value[3]) {
                width = (GLfloat)widthInt;
                height = (GLfloat)heightInt;
                _width = 1 / width;
                _height = 1 / height;
                if (migldebug) ALOGE("migldebug:modifyRenderPrecisionYuanshenShaderUniform, su: [%f, %f, %f, %f] -> [%f, %f, %f, %f]",
                    value[0], value[1], value[2], value[3], _width, _height, width, height);
                value[0] = _width;
                value[1] = _height;
                value[2] = width;
                value[3] = height;
            }
        }
    }
}

void modifyAFValuePubg(GLenum pname, GLint& param) {
    if (mEnableAFModify) {
        if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT && param > mTargetAFValue) {
            if(migldebug) ALOGE("migldebug:modifyAFValuePubg, af: %d -> %d", param, mTargetAFValue);
            param = mTargetAFValue;
        }
    }
}

void modifyShadingRate() {
   if (glShadingRateQCOM != NULL && mEnableShadingRate == true && mShadingRate > 0) {
        //There are several options for shading rate here, which can be expanded according to the scene.
    switch(mShadingRate) {
        case 1:
            glShadingRateQCOM(GL_SHADING_RATE_1X2_PIXELS_QCOM);
            break;
        case 2:
            glShadingRateQCOM(GL_SHADING_RATE_2X1_PIXELS_QCOM);
            break;
        case 3:
            glShadingRateQCOM(GL_SHADING_RATE_2X2_PIXELS_QCOM);
            break;
        case 4:
            glShadingRateQCOM(GL_SHADING_RATE_4X2_PIXELS_QCOM);
            break;
        case 5:
            glShadingRateQCOM(GL_SHADING_RATE_4X4_PIXELS_QCOM);
            break;
        default:
            ALOGE("migl:undefined shading rate");
        }
        if(migldebug) ALOGE("migldebug:modifyShadingRate, mShadingRate: %d", mShadingRate);
    }
}

void updateTexSizeArr() {
    if(mEnableYSSceneRecognize && mSceneRecognizer != nullptr) {
        GLint width, height;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        mSceneRecognizer->updateTexSizeArr(width, height);
    }
}

void checkBossPattern() {
    if(mEnableYSSceneRecognize && mSceneRecognizer != nullptr)
        mSceneRecognizer->checkBossPattern();
}

void updateEleCountArr(const GLsizei& count) {
    if(mEnableYSSceneRecognize && mSceneRecognizer != nullptr)
        mSceneRecognizer->updateEleCountArr(count);
}

}
