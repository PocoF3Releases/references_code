/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "MiuiRenderEngineLowEffect"
#define ATRACE_TAG ATRACE_TAG_BLUR
#include <utils/Log.h>

#include <math.h>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <unistd.h>

#include "MiuiRenderEngineLowEffect.h"

namespace android {
using namespace uirenderer;

static int sInitFbo = 0;
static bool enablePPMs = false;
MMesh MiuiRenderEngineLowEffect::sSimpleMesh(MMesh::TRIANGLE_FAN, 4, 2, 2);

/**
 * return true if no GL Error
 *
 * #ifndef GL_INVALID_ENUM
 * #define GL_NO_ERROR                       0
 * #define GL_INVALID_ENUM                   0x0500
 * #define GL_INVALID_VALUE                  0x0501
 * #define GL_INVALID_OPERATION              0x0502
 * #define GL_STACK_OVERFLOW                 0x0503
 * #define GL_STACK_UNDERFLOW                0x0504
 * #define GL_OUT_OF_MEMORY                  0x0505
 */
void MiuiRenderEngineLowEffect::checkErrors(const char* msg) {
    do {
        // there could be more than one error flag
        GLenum error = glGetError();
        if (error == GL_NO_ERROR) break;
        LOG("MiuiRenderEngineLowEffect checkErrors 0x%04x %s", int(error), msg);
    } while (true);
}

void MiuiRenderEngineLowEffect::writePPM(const char* basename, uint32_t width, uint32_t height) {

    std::vector<GLubyte> pixels(width * height * 4);
    std::vector<GLubyte> outBuffer(width * height * 3);

    // TODO(courtneygo): We can now have float formats, need
    // to remove this code or update to support.
    // Make returned pixels fit in uint32_t, one byte per component
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    checkErrors("writePPM");

    std::string filename;
    filename.clear();
    filename.append(basename).append(".ppm");
    std::ofstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        LOG("checkGlError Unable to open file: %s", filename.c_str());
        LOG("checkGlError You may need to do: \"adb shell setenforce 0\" to enable "
            "surfaceflinger to write debug images");
        return;
    }

    file << "P6\n";
    file << width << "\n";
    file << height << "\n";
    file << 255 << "\n";

    auto ptr = reinterpret_cast<char*>(pixels.data());
    auto outPtr = reinterpret_cast<char*>(outBuffer.data());
    for (int y = height - 1; y >= 0; y--) {
        char* data = ptr + y * width * sizeof(uint32_t);

        for (GLuint x = 0; x < width; x++) {
            // Only copy R, G and B components
            outPtr[0] = data[0];
            outPtr[1] = data[1];
            outPtr[2] = data[2];
            data += sizeof(uint32_t);
            outPtr += 3;
        }
    }
    file.write(reinterpret_cast<char*>(outBuffer.data()), outBuffer.size());
}

void MiuiRenderEngineLowEffect::enableDumpPPM(){
    enablePPMs = true;
}

void MiuiRenderEngineLowEffect::disableDumpPPM(){
    enablePPMs = false;
}

static void setupVectors(MMesh::VertexArray<mvec2>& vectors,
                         float x1, float x2, float y1, float y2) {
    vectors[0] = mvec2(x1, y1);
    vectors[1] = mvec2(x1, y2);
    vectors[2] = mvec2(x2, y2);
    vectors[3] = mvec2(x2, y1);
}

void MiuiRenderEngineLowEffect::createFbo(FrameBufferObject &mFrameBufferObject) {
    glGenFramebuffers(1, &mFrameBufferObject.framebufferId);
}

void MiuiRenderEngineLowEffect::createTexture(TextureObject &mTextureObject) {
    glGenTextures(1, &mTextureObject.textureId);

    if (mTextureObject.textureId != 0) {
        glBindTexture(GL_TEXTURE_2D, mTextureObject.textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTextureObject.width, mTextureObject.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void MiuiRenderEngineLowEffect::resizeTexture(TextureObject &mTextureObject) {
    glBindTexture(GL_TEXTURE_2D, mTextureObject.textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mTextureObject.width, mTextureObject.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void MiuiRenderEngineLowEffect::bindTextureToFbo(FrameBufferObject &mFrameBufferObject, TextureObject &mTextureObject) {
    if (mTextureObject.textureId != 0) {
        mFrameBufferObject.textureId = mTextureObject.textureId;
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferObject.framebufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureObject.textureId, 0);
    }
}

void MiuiRenderEngineLowEffect::bindFbo(FrameBufferObject &mFrameBufferObject) {
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferObject.framebufferId);
}

void MiuiRenderEngineLowEffect::setViewport(size_t left, size_t top, size_t width, size_t height) {
    glViewport(left, top, width, height);
}

void MiuiRenderEngineLowEffect::deleteFbo(uint32_t fbName) {
    glDeleteFramebuffers(1, &fbName);
}

void MiuiRenderEngineLowEffect::deleteTexture(uint32_t texName) {
    glDeleteTextures(1, &texName);
}

void MiuiRenderEngineLowEffect::clearWithColor(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void MiuiRenderEngineLowEffect::drawMeshSimple(const MMesh& mesh) {
    glEnableVertexAttribArray(eAttrLocTexCoords);
    glVertexAttribPointer(eAttrLocTexCoords,
        mesh.getTexCoordsSize(),
        GL_FLOAT, GL_FALSE,
        mesh.getByteStride(),
        mesh.getTexCoords());

    glEnableVertexAttribArray(eAttrLocPosition);
    glVertexAttribPointer(eAttrLocPosition,
        mesh.getVertexSize(),
        GL_FLOAT, GL_FALSE,
        mesh.getByteStride(),
        mesh.getPositions());

    glDrawArrays(mesh.getPrimitive(), 0, mesh.getVertexCount());
    glDisableVertexAttribArray(eAttrLocTexCoords);
}

void MiuiRenderEngineLowEffect::releaseResource(DrawObject &mDrawInfo) {
    if (mDrawInfo.mFboTexture.textureId != 0) {
        deleteFbo(mDrawInfo.mFbo.framebufferId);
        deleteTexture(mDrawInfo.mFboTexture.textureId);
        deleteTexture(mDrawInfo.mTempTexture.textureId);
        mDrawInfo.mFbo.framebufferId = 0;
        mDrawInfo.mFboTexture.textureId = 0;
        mDrawInfo.mTempTexture.textureId = 0;
    }
}

void MiuiRenderEngineLowEffect::releaseResource() {
    if (mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.textureId != 0) {
        deleteFbo(mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo.framebufferId);
        deleteTexture(mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.textureId);
        deleteTexture(mViewFunctor.mViewDrawInfo.mDrawInfo.mTempTexture.textureId);
        deleteTexture(mViewFunctor.mViewDrawInfo.mDrawInfo.mBlendTexture.textureId);
        mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo.framebufferId = 0;
        mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.textureId = 0;
        mViewFunctor.mViewDrawInfo.mDrawInfo.mTempTexture.textureId = 0;
        mViewFunctor.mViewDrawInfo.mDrawInfo.mBlendTexture.textureId = 0;
    }
}

void MiuiRenderEngineLowEffect::computeBlurParam(FunctorData & mFunctor, DrawGlInfo *c_drawGlInfo) {
    memcpy(&mViewFunctor.mViewDrawInfo.mGlInfo, c_drawGlInfo, sizeof(DrawGlInfo));
    if (!isChildrenGLInfo()) {
        memcpy(&mViewFunctor.mViewDrawInfo.mParentGlInfo, c_drawGlInfo, sizeof(DrawGlInfo));
    }

    mViewFunctor.mViewDrawInfo.mDrawInfo.mBlurRatio = mFunctor.mBlurRatio;
    mViewFunctor.mViewDrawInfo.mDrawInfo.mBlurMode = mFunctor.mBlurMode;

    memcpy(&mViewFunctor.mViewDrawInfo.mBlurCornerRadii, mFunctor.mBlurCornerRadii, sizeof(mFunctor.mBlurCornerRadii));
}

bool MiuiRenderEngineLowEffect::isChildrenGLInfo() {
    if (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipLeft == mViewFunctor.mViewDrawInfo.mGlInfo.clipLeft
            && mViewFunctor.mViewDrawInfo.mParentGlInfo.clipRight == mViewFunctor.mViewDrawInfo.mGlInfo.clipRight
            && mViewFunctor.mViewDrawInfo.mParentGlInfo.clipTop == mViewFunctor.mViewDrawInfo.mGlInfo.clipTop
            && mViewFunctor.mViewDrawInfo.mParentGlInfo.clipBottom == mViewFunctor.mViewDrawInfo.mGlInfo.clipBottom) {
        return false;
    }

    return mViewFunctor.mViewDrawInfo.mParentGlInfo.clipLeft < mViewFunctor.mViewDrawInfo.mParentGlInfo.clipRight
                && mViewFunctor.mViewDrawInfo.mParentGlInfo.clipTop < mViewFunctor.mViewDrawInfo.mParentGlInfo.clipBottom
                && mViewFunctor.mViewDrawInfo.mParentGlInfo.clipLeft <= mViewFunctor.mViewDrawInfo.mGlInfo.clipLeft
                && mViewFunctor.mViewDrawInfo.mParentGlInfo.clipTop <= mViewFunctor.mViewDrawInfo.mGlInfo.clipTop
                && mViewFunctor.mViewDrawInfo.mParentGlInfo.clipRight >= mViewFunctor.mViewDrawInfo.mGlInfo.clipRight
                && mViewFunctor.mViewDrawInfo.mParentGlInfo.clipBottom >= mViewFunctor.mViewDrawInfo.mGlInfo.clipBottom;
}



void MiuiRenderEngineLowEffect::computeBlurParam(float blurRatio,float blurMode) {
    mRatio = blurRatio;
    mMode = blurMode;
}

void MiuiRenderEngineLowEffect::deletePrograms() {
    if (mSimpleProgram != nullptr) {
        mSimpleProgram->deleteProgram();
        mSimpleProgram = nullptr;
    }

    if (mSaturationProgram != nullptr) {
        mSaturationProgram->deleteProgram();
        mSaturationProgram = nullptr;
    }

    if (mBlurXProgram != nullptr) {
        mBlurXProgram->deleteProgram();
        mBlurXProgram = nullptr;
    }

    if (mBlurYProgram != nullptr) {
        mBlurYProgram->deleteProgram();
        mBlurYProgram = nullptr;
    }

    if (mBlendColorProgram != nullptr) {
        mBlendColorProgram->deleteProgram();
        mBlendColorProgram = nullptr;
    }

    if (mRoundCornerProgram != nullptr) {
        mRoundCornerProgram->deleteProgram();
        mRoundCornerProgram = nullptr;
    }
}

void MiuiRenderEngineLowEffect::prepareTexture(ViewObject &mViewDrawInfo) {
    if (!mViewDrawInfo.mChildRedraw) {
        if ((mVpWidth != mViewDrawInfo.mDisplayTexture.width) ||
                (mVpHeight != mViewDrawInfo.mDisplayTexture.height)) {
            mViewDrawInfo.mDisplayTexture.width = mVpWidth;
            mViewDrawInfo.mDisplayTexture.height = mVpHeight;

            if (mViewDrawInfo.mDisplayTexture.textureId != 0) {
                resizeTexture(mViewDrawInfo.mDisplayTexture);
            } else {
                createTexture(mViewDrawInfo.mDisplayTexture);
            }
        }
    }
}


void MiuiRenderEngineLowEffect::initMVPMatrix(ViewObject &mViewDrawInfo) {
    Math::setIdentityM(mViewDrawInfo.mTexMatrix, 0);
    Math::setIdentityM(mViewDrawInfo.mModelMatrix, 0);
    Math::setIdentityM(mViewDrawInfo.mViewMatrix, 0);
    Math::setIdentityM(mViewDrawInfo.mProjMatrix, 0);

    Math::scaleM(mViewDrawInfo.mModelMatrix, 0, mVpWidth, mVpHeight, 1.0f);
    Math::orthoM(mViewDrawInfo.mProjMatrix, 0, 0, mVpWidth, 0, mVpHeight, 0, 1.0f);

    Math::multiplyMM(mViewDrawInfo.mMVPMatrix, mViewDrawInfo.mViewMatrix, mViewDrawInfo.mModelMatrix);
    Math::multiplyMM(mViewDrawInfo.mMVPMatrix, mViewDrawInfo.mProjMatrix, mViewDrawInfo.mMVPMatrix);

    Math::setIdentityM(mViewDrawInfo.mModelMatrix, 0);
    Math::translateM(mViewDrawInfo.mModelMatrix, 0,
            mViewDrawInfo.mGlInfo.clipLeft,
            mViewDrawInfo.mGlInfo.height - mViewDrawInfo.mGlInfo.clipBottom, 0);

    Math::scaleM(mViewDrawInfo.mModelMatrix, 0, mVpWidth, mVpHeight, 1.0f);
    Math::setIdentityM(mViewDrawInfo.mProjMatrix, 0);
    Math::orthoM(mViewDrawInfo.mProjMatrix, 0, 0,
            mViewDrawInfo.mGlInfo.width, 0,
            mViewDrawInfo.mGlInfo.height, 0, 1.0f);

    Math::multiplyMM(mViewDrawInfo.mScreenMVPMatrix, mViewDrawInfo.mViewMatrix, mViewDrawInfo.mModelMatrix);
    Math::multiplyMM(mViewDrawInfo.mScreenMVPMatrix, mViewDrawInfo.mProjMatrix, mViewDrawInfo.mScreenMVPMatrix);
}

void MiuiRenderEngineLowEffect::prepareFbo() {
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &sInitFbo);
}

void MiuiRenderEngineLowEffect::onPostBlur() {
    glBindFramebuffer(GL_FRAMEBUFFER, sInitFbo);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void MiuiRenderEngineLowEffect::checkClipSize(DrawGlInfo& info) {
    if (info.height != 0 && (info.clipBottom - info.clipTop >= info.height)) {
        info.clipTop = info.clipTop + 1;
    }
    if (info.width != 0 && (info.clipRight - info.clipLeft >= info.width)) {
        info.clipLeft = info.clipLeft + 1;
    }
}

void MiuiRenderEngineLowEffect::copyTextureFromScreen(ViewObject mViewDrawInfo) {
    if (mViewDrawInfo.mDisplayTexture.textureId != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mViewDrawInfo.mDisplayTexture.textureId);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                mViewDrawInfo.mGlInfo.clipLeft,
                mViewDrawInfo.mGlInfo.height - mViewDrawInfo.mGlInfo.clipBottom,
                mViewDrawInfo.mDisplayTexture.width, mViewDrawInfo.mDisplayTexture.height);
    }
}


void MiuiRenderEngineLowEffect::selectDisplayTexture(ViewObject &mViewDrawInfo) {
    copyTextureFromScreen(mViewDrawInfo);
}

bool MiuiRenderEngineLowEffect::prepareBlurDrawable() {

    char value[MPROPERTY_VALUE_MAX];
    __system_property_get("persist.sys.real_blur_radius", value);
    int blurRadius = atoi(value);
    if (blurRadius <= 0) blurRadius = 8;

    EGLContext context = eglGetCurrentContext();
    if (context == EGL_NO_CONTEXT) {
        LOG("This thread has no EGLContext.");
        return false;
    }
    checkClipSize(mViewFunctor.mViewDrawInfo.mGlInfo);
    mVpWidth = mViewFunctor.mViewDrawInfo.mGlInfo.clipRight - mViewFunctor.mViewDrawInfo.mGlInfo.clipLeft;
    mVpHeight = mViewFunctor.mViewDrawInfo.mGlInfo.clipBottom - mViewFunctor.mViewDrawInfo.mGlInfo.clipTop;

    mLayerDrawInfo.mDrawInfo.mBlurTextureWidth = mVpWidth >> SCALE_SHIFT_BIT;
    mLayerDrawInfo.mDrawInfo.mBlurTextureHeight = mVpHeight >> SCALE_SHIFT_BIT;

    if (mViewFunctor.mViewDrawInfo.mNeedRelink) {
        deletePrograms();

        mSaturationProgram = std::make_unique<MiuiProgram>();
        mSaturationProgram->create(mShaderUtil.getVertexShader(), mShaderUtil.saturationFragmentShader());

        mBlurXProgram = std::make_unique<MiuiProgram>();
        String8 mblurXProgramString;
        mShaderUtil.getFragmentShader(true, blurRadius, mLayerDrawInfo.mDrawInfo.mBlurTextureWidth, mblurXProgramString);
        mBlurXProgram->create(mShaderUtil.getVertexShader(), mblurXProgramString.c_str());

        mBlurYProgram = std::make_unique<MiuiProgram>();
        String8 mblurYProgramString;
        mShaderUtil.getFragmentShader(false, blurRadius, mLayerDrawInfo.mDrawInfo.mBlurTextureHeight, mblurYProgramString);
        mBlurYProgram->create(mShaderUtil.getVertexShader(), mblurYProgramString.c_str());

        mBlendColorProgram = std::make_unique<MiuiProgram>();
        mBlendColorProgram->create(mShaderUtil.getVertexCode(), mShaderUtil.getBlendColorFragmentCode());

        mRoundCornerProgram = std::make_unique<MiuiProgram>();
        mRoundCornerProgram->create(mShaderUtil.getVertexCode(), mShaderUtil.getRoundCornerCode());

        mViewFunctor.mViewDrawInfo.mNeedRelink = false;
    }

    prepareFbo();
    prepareTexture(mViewFunctor.mViewDrawInfo);
    initMVPMatrix(mViewFunctor.mViewDrawInfo);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    selectDisplayTexture(mViewFunctor.mViewDrawInfo);

    return true;
}

void MiuiRenderEngineLowEffect::prepareViewFunctor() {
    mViewFunctor.isEnable = true;
    mViewFunctor.mViewDrawInfo.mNeedRelink = false;
    mViewFunctor.mViewDrawInfo.mMixMode = -1;
    mViewFunctor.mViewDrawInfo.mMixColor = 0;
    float defaultCornerRadii[4] = {0, 0,0,0};
    memcpy(&mViewFunctor.mViewDrawInfo.mBlurCornerRadii, defaultCornerRadii, sizeof(defaultCornerRadii));
    mViewFunctor.mViewDrawInfo.mDrawInfo.mBlurRatio = 1.0f;
    mViewFunctor.mViewDrawInfo.mDrawInfo.mBlurMode = 0;
    mViewFunctor.mViewDrawInfo.mDrawInfo.mBlurRadius = 0;
    mViewFunctor.mViewDrawInfo.mChildRedraw = false;
    mViewFunctor.mViewDrawInfo.mDisplayTexture.textureId = 0;
    mViewFunctor.mViewDrawInfo.mDisplayTexture.width = 0;
    mViewFunctor.mViewDrawInfo.mDisplayTexture.height = 0;
    mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.textureId = 0;
}

void MiuiRenderEngineLowEffect::beginDrawToFbo() {
    if (mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.textureId == 0) {
        createTexture(mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture);
        createTexture(mViewFunctor.mViewDrawInfo.mDrawInfo.mTempTexture);
        createTexture(mViewFunctor.mViewDrawInfo.mDrawInfo.mBlendTexture);
        createFbo(mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo);
    }

    bindFbo(mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo);
    bindTextureToFbo(mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo, mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture);
}

void MiuiRenderEngineLowEffect::endDrawToFbo() {
    setViewport(0, 0, mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.width, mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.height);

    glUseProgram(mSaturationProgram->getProgramId());

    int mIncreaseSaturationLoc = glGetUniformLocation(mSaturationProgram->getProgramId(), "increaseSaturation");
    glUniform1f(mIncreaseSaturationLoc, mIncreaseSaturation);
    int mBlackLoc = glGetUniformLocation(mSaturationProgram->getProgramId(), "black");
    glUniform1f(mBlackLoc, mBlack);
    int mModeLoc = glGetUniformLocation(mSaturationProgram->getProgramId(), "mode");
    glUniform1i(mModeLoc, mViewFunctor.mViewDrawInfo.mDrawInfo.mBlurMode);

    int textureUniformId = glGetUniformLocation(mSaturationProgram->getProgramId(), "sampler1");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mViewFunctor.mViewDrawInfo.mDisplayTexture.textureId);
    glUniform1i(textureUniformId, 0);

    drawMeshSimple(sSimpleMesh);

    checkErrors("endDrawToFbo");
}

void MiuiRenderEngineLowEffect::makeFboBlur() {
    setViewport(0, 0, mViewFunctor.mViewDrawInfo.mDrawInfo.mTempTexture.width, mViewFunctor.mViewDrawInfo.mDrawInfo.mTempTexture.height);
    // make blur by x axis
    glUseProgram(mBlurXProgram->getProgramId());
    bindTextureToFbo(mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo, mViewFunctor.mViewDrawInfo.mDrawInfo.mTempTexture);

    int textureUniformId = glGetUniformLocation(mBlurXProgram->getProgramId(), "sampler");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mViewFunctor.mViewDrawInfo.mDisplayTexture.textureId);
    glUniform1i(textureUniformId, 0);
    drawMeshSimple(sSimpleMesh);

    // make blur by y axis
    glUseProgram(mBlurYProgram->getProgramId());
    bindTextureToFbo(mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo, mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture);

    textureUniformId = glGetUniformLocation(mBlurYProgram->getProgramId(), "sampler");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mViewFunctor.mViewDrawInfo.mDrawInfo.mTempTexture.textureId);
    glUniform1i(textureUniformId, 0);
    drawMeshSimple(sSimpleMesh);

    checkErrors("makeFboBlur");
}

bool MiuiRenderEngineLowEffect::drawBlendColor(int color, int mode, bool isTwo) {
    setViewport(0, 0, mViewFunctor.mViewDrawInfo.mDrawInfo.mBlendTexture.width, mViewFunctor.mViewDrawInfo.mDrawInfo.mBlendTexture.height);

    glUseProgram(mBlendColorProgram->getProgramId());

    int positionId = glGetAttribLocation(mBlendColorProgram->getProgramId(), "aPosition");
    glEnableVertexAttribArray(positionId);
    glVertexAttribPointer(positionId, COORDS_PER_VERTEX, GL_FLOAT, false, VERTEX_STRIDE, (void*)squareCoords);

    int mvpMatrixId = glGetUniformLocation(mBlendColorProgram->getProgramId(), "uMVPMatrix");

    glUniformMatrix4fv(mvpMatrixId, 1, false, (float *)mViewFunctor.mViewDrawInfo.mMVPMatrix);

    int texMatrixId = glGetUniformLocation(mBlendColorProgram->getProgramId(), "uTexMatrix");
    glUniformMatrix4fv(texMatrixId, 1, false, (float *)mViewFunctor.mViewDrawInfo.mTexMatrix);

    int texCoordId = glGetAttribLocation(mBlendColorProgram->getProgramId(), "aTexCoord");
    glEnableVertexAttribArray(texCoordId);
    glVertexAttribPointer(texCoordId, 2, GL_FLOAT, false, 0, (void*)mTexHorizontalCoords);

    int mixPercentId = glGetUniformLocation(mBlendColorProgram->getProgramId(), "mixPercent");
    glUniform1f(mixPercentId, 1.0f);

    int mixColorId = glGetUniformLocation(mBlendColorProgram->getProgramId(), "vMixColor");
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = (color) & 0xFF;
    int a = (((unsigned)color) >> 24) & 0xFF;
    float mixColor[4] = {(float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f};
    glUniform4fv(mixColorId, 1, mixColor);

    int mixModeId = glGetUniformLocation(mBlendColorProgram->getProgramId(), "vMixMode");
    glUniform1i(mixModeId, mode);

    if (isTwo) {
        bindTextureToFbo(mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo, mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture);
    } else {
        bindTextureToFbo(mViewFunctor.mViewDrawInfo.mDrawInfo.mFbo, mViewFunctor.mViewDrawInfo.mDrawInfo.mBlendTexture);
    }
    int textureUniformId = glGetUniformLocation(mBlendColorProgram->getProgramId(), "uTexture");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, isTwo ? mViewFunctor.mViewDrawInfo.mDrawInfo.mBlendTexture.textureId : mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.textureId);
    glUniform1i(textureUniformId, 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void *)drawOrder);
    checkErrors("drawBlendColor");
    return true;
}

bool MiuiRenderEngineLowEffect::drawRoundCorner(bool isTwo) {
    glViewport(0, 0, mViewFunctor.mViewDrawInfo.mGlInfo.width, mViewFunctor.mViewDrawInfo.mGlInfo.height);

    glUseProgram(mRoundCornerProgram->getProgramId());

    int positionId = glGetAttribLocation(mRoundCornerProgram->getProgramId(), "aPosition");
    glEnableVertexAttribArray(positionId);
    glVertexAttribPointer(positionId, COORDS_PER_VERTEX, GL_FLOAT, false, VERTEX_STRIDE, (void*)squareCoords);

    int mvpMatrixId = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "uMVPMatrix");
    glUniformMatrix4fv(mvpMatrixId, 1, false, (float *)mViewFunctor.mViewDrawInfo.mScreenMVPMatrix);

    int texMatrixId = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "uTexMatrix");
    glUniformMatrix4fv(texMatrixId, 1, false, (float *)mViewFunctor.mViewDrawInfo.mTexMatrix);

    int texCoordId = glGetAttribLocation(mRoundCornerProgram->getProgramId(), "aTexCoord");
    glEnableVertexAttribArray(texCoordId);
    glVertexAttribPointer(texCoordId, 2, GL_FLOAT, false, 0, (void*)mTexHorizontalCoords);

    glBindFramebuffer(GL_FRAMEBUFFER, sInitFbo);

    int rtCornerRadius = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "rtCornerRadius");
    int rbCornerRadius = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "rbCornerRadius");
    int ltCornerRadius = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "ltCornerRadius");
    int lbCornerRadius = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "lbCornerRadius");

    int cropSize = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "cropSize");

    int rtCornerCenter = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "rtCornerCenter");
    int rbCornerCenter = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "rbCornerCenter");
    int lbCornerCenter = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "lbCornerCenter");
    int ltCornerCenter = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "ltCornerCenter");

    float rtCenterX = 0.0f;
    float rtCenterY = 0.0f;

    float rbCenterX = 0.0f;
    float rbCenterY = 0.0f;

    float lbCenterX = 0.0f;
    float lbCenterY = 0.0f;

    float ltCenterX = 0.0f;
    float ltCenterY = 0.0f;

    if (mVpWidth > mVpHeight) {
        glUniform1f(rtCornerRadius, mViewFunctor.mViewDrawInfo.mBlurCornerRadii[1] / (float) mVpHeight);
        glUniform1f(rbCornerRadius, mViewFunctor.mViewDrawInfo.mBlurCornerRadii[2] / (float) mVpHeight);
        glUniform1f(ltCornerRadius, mViewFunctor.mViewDrawInfo.mBlurCornerRadii[0] / (float) mVpHeight);
        glUniform1f(lbCornerRadius, mViewFunctor.mViewDrawInfo.mBlurCornerRadii[3] / (float) mVpHeight);
        glUniform2f(cropSize, (float) mVpWidth / (float) mVpHeight, (float) mVpHeight / (float) mVpHeight);

        rtCenterX = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[1]
            - (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipRight - mViewFunctor.mViewDrawInfo.mGlInfo.clipRight)) / (float) mVpHeight) * ((float) mVpHeight / (float) mVpWidth);
        rtCenterY = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[1]
            - (mViewFunctor.mViewDrawInfo.mGlInfo.clipTop - mViewFunctor.mViewDrawInfo.mParentGlInfo.clipTop)) / (float) mVpHeight) * 1.0f;

        rbCenterX = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[2]
            - (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipRight - mViewFunctor.mViewDrawInfo.mGlInfo.clipRight)) / (float) mVpHeight) * ((float) mVpHeight / (float) mVpWidth);
        rbCenterY = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[2]
            - (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipBottom - mViewFunctor.mViewDrawInfo.mGlInfo.clipBottom)) / (float) mVpHeight) * 1.0f;

        lbCenterX = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[3]
            - (mViewFunctor.mViewDrawInfo.mGlInfo.clipLeft - mViewFunctor.mViewDrawInfo.mParentGlInfo.clipLeft)) / (float) mVpHeight) * ((float) mVpHeight / (float) mVpWidth);
        lbCenterY = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[3]
            - (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipBottom - mViewFunctor.mViewDrawInfo.mGlInfo.clipBottom)) / (float) mVpHeight) * 1.0f;


        ltCenterX = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[0]
            - (mViewFunctor.mViewDrawInfo.mGlInfo.clipLeft - mViewFunctor.mViewDrawInfo.mParentGlInfo.clipLeft)) / (float) mVpHeight) * ((float) mVpHeight / (float) mVpWidth);
        ltCenterY = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[0]
            - (mViewFunctor.mViewDrawInfo.mGlInfo.clipTop - mViewFunctor.mViewDrawInfo.mParentGlInfo.clipTop)) / (float) mVpHeight) * 1.0f;
    } else {

        glUniform1f(rtCornerRadius, mViewFunctor.mViewDrawInfo.mBlurCornerRadii[1] / (float) mVpWidth);
        glUniform1f(rbCornerRadius, mViewFunctor.mViewDrawInfo.mBlurCornerRadii[2] / (float) mVpWidth);
        glUniform1f(ltCornerRadius, mViewFunctor.mViewDrawInfo.mBlurCornerRadii[0] / (float) mVpWidth);
        glUniform1f(lbCornerRadius, mViewFunctor.mViewDrawInfo.mBlurCornerRadii[3] / (float) mVpWidth);
        glUniform2f(cropSize, (float) mVpWidth / (float) mVpWidth, (float) mVpHeight / (float) mVpWidth);
        rtCenterX = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[1]
            - (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipRight - mViewFunctor.mViewDrawInfo.mGlInfo.clipRight)) / (float) mVpWidth) * 1.0f;
        rtCenterY = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[1]
            - (mViewFunctor.mViewDrawInfo.mGlInfo.clipTop - mViewFunctor.mViewDrawInfo.mParentGlInfo.clipTop)) / (float) mVpWidth) * ((float) mVpWidth / (float) mVpHeight);

        rbCenterX = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[2]
            - (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipRight - mViewFunctor.mViewDrawInfo.mGlInfo.clipRight)) / (float) mVpWidth) * 1.0f;
        rbCenterY = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[2]
            - (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipBottom - mViewFunctor.mViewDrawInfo.mGlInfo.clipBottom)) / (float) mVpWidth) * ((float) mVpWidth / (float) mVpHeight);

        lbCenterX = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[3]
            - (mViewFunctor.mViewDrawInfo.mGlInfo.clipLeft - mViewFunctor.mViewDrawInfo.mParentGlInfo.clipLeft)) / (float) mVpWidth) * 1.0f;
        lbCenterY = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[3]
            - (mViewFunctor.mViewDrawInfo.mParentGlInfo.clipBottom - mViewFunctor.mViewDrawInfo.mGlInfo.clipBottom)) / (float) mVpWidth) * ((float) mVpWidth / (float) mVpHeight);


        ltCenterX = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[0]
            - (mViewFunctor.mViewDrawInfo.mGlInfo.clipLeft - mViewFunctor.mViewDrawInfo.mParentGlInfo.clipLeft)) / (float) mVpWidth) * 1.0f;
        ltCenterY = ((float) (mViewFunctor.mViewDrawInfo.mBlurCornerRadii[0]
            - (mViewFunctor.mViewDrawInfo.mGlInfo.clipTop - mViewFunctor.mViewDrawInfo.mParentGlInfo.clipTop)) / (float) mVpWidth) * ((float) mVpWidth / (float) mVpHeight);
    }

    glUniform2f(rtCornerCenter, 1 - rtCenterX, 1 - rtCenterY);
    glUniform2f(rbCornerCenter, 1 - rbCenterX, rbCenterY);
    glUniform2f(lbCornerCenter, lbCenterX, lbCenterY);
    glUniform2f(ltCornerCenter, ltCenterX, 1 - ltCenterY);

    int textureUniformId = glGetUniformLocation(mRoundCornerProgram->getProgramId(), "uTexture");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, isTwo ? mViewFunctor.mViewDrawInfo.mDrawInfo.mFboTexture.textureId : mViewFunctor.mViewDrawInfo.mDrawInfo.mBlendTexture.textureId);
    glUniform1i(textureUniformId, 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void *)drawOrder);

    checkErrors("drawRoundCorner");
    return true;
}

void MiuiRenderEngineLowEffect::beginDrawToFbo(LayerObject &mLayerDrawInfo) {
    prepareFbo();

    if (mLayerDrawInfo.mDrawInfo.mFboTexture.textureId == 0) {
        createTexture(mLayerDrawInfo.mDrawInfo.mFboTexture);
        createTexture(mLayerDrawInfo.mDrawInfo.mTempTexture);
        createFbo(mLayerDrawInfo.mDrawInfo.mFbo);
    }

    bindFbo(mLayerDrawInfo.mDrawInfo.mFbo);

    if (mMiddleTexturesCount) {
        // draw to biggest middle texture
        bindTextureToFbo(mLayerDrawInfo.mDrawInfo.mFbo, *mMiddleTextures);
        setViewport(0, 0, mMiddleTextures->width, mMiddleTextures->height);
    } else {
        // draw to FboTexture
        bindTextureToFbo(mLayerDrawInfo.mDrawInfo.mFbo, mLayerDrawInfo.mDrawInfo.mFboTexture);
        setViewport(0, 0, mLayerDrawInfo.mDrawInfo.mFboTexture.width, mLayerDrawInfo.mDrawInfo.mFboTexture.height);
    }
    clearWithColor(0, 0, 0, 0);
}

void MiuiRenderEngineLowEffect::endDrawToFbo(LayerObject &mLayerDrawInfo) {

    if (mMiddleTexturesCount > 0) {
        glUseProgram(mSimpleProgram->getProgramId());

        // biggest middle texture draw to smallest middle texture
        TextureObject *currMiddleTexture = mMiddleTextures;
        TextureObject *nextMiddleTexture = mMiddleTextures + 1;

        const int sizeMinusOne = mMiddleTexturesCount - 1;
        for (int i=0; i<sizeMinusOne; i++) {
            if (nextMiddleTexture->height < mLayerDrawInfo.mDrawInfo.mBlurTextureHeight) {
                break;
            }
            setViewport(0, 0, nextMiddleTexture->width, nextMiddleTexture->height);
            bindTextureToFbo(mLayerDrawInfo.mDrawInfo.mFbo, *nextMiddleTexture);

            int textureUniformId = glGetUniformLocation(mSimpleProgram->getProgramId(), "sampler1");
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, currMiddleTexture->textureId);
            glUniform1i(textureUniformId, 0);

            drawMeshSimple(sSimpleMesh);

            currMiddleTexture++;
            nextMiddleTexture++;
        }

        glUseProgram(mSaturationProgram->getProgramId());

        int mIncreaseSaturationLoc = glGetUniformLocation(mSaturationProgram->getProgramId(), "increaseSaturation");
        glUniform1f(mIncreaseSaturationLoc, mIncreaseSaturation);
        int mBlackLoc = glGetUniformLocation(mSaturationProgram->getProgramId(), "black");
        glUniform1f(mBlackLoc, mBlack);

        int mModeLoc = glGetUniformLocation(mSaturationProgram->getProgramId(), "mode");
        glUniform1i(mModeLoc, mLayerDrawInfo.mDrawInfo.mBlurMode);

        setViewport(0, 0, mLayerDrawInfo.mDrawInfo.mBlurTextureWidth, mLayerDrawInfo.mDrawInfo.mBlurTextureHeight);
        bindTextureToFbo(mLayerDrawInfo.mDrawInfo.mFbo, mLayerDrawInfo.mDrawInfo.mFboTexture);
        //smallest middle texture draw to FboTexture
        int textureUniformId = glGetUniformLocation(mSaturationProgram->getProgramId(), "sampler1");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currMiddleTexture->textureId);
        glUniform1i(textureUniformId, 0);

        drawMeshSimple(sSimpleMesh);

    } else {
        // TODO: increase saturation use Blur::sSaturationProgramId
    }
}

void MiuiRenderEngineLowEffect::makeFboBlur(LayerObject &mLayerDrawInfo) {

    //prepare
    setViewport(0, 0, mLayerDrawInfo.mDrawInfo.mBlurTextureWidth, mLayerDrawInfo.mDrawInfo.mBlurTextureHeight);

    // make blur by x axis
    glUseProgram(mBlurXProgram->getProgramId());
    bindTextureToFbo(mLayerDrawInfo.mDrawInfo.mFbo, mLayerDrawInfo.mDrawInfo.mTempTexture);

    int textureUniformId = glGetUniformLocation(mBlurXProgram->getProgramId(), "sampler");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mLayerDrawInfo.mDrawInfo.mFboTexture.textureId);
    glUniform1i(textureUniformId, 0);
    drawMeshSimple(sSimpleMesh);

    // make blur by y axis
    glUseProgram(mBlurYProgram->getProgramId());
    bindTextureToFbo(mLayerDrawInfo.mDrawInfo.mFbo, mLayerDrawInfo.mDrawInfo.mFboTexture);

    textureUniformId = glGetUniformLocation(mBlurXProgram->getProgramId(), "sampler");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mLayerDrawInfo.mDrawInfo.mTempTexture.textureId);
    glUniform1i(textureUniformId, 0);
    drawMeshSimple(sSimpleMesh);
}

MiuiRenderEngineLowEffect* MiuiRenderEngineLowEffect::create(float density, uint32_t screenWidth, uint32_t screenHeight) {
    // initialize the MiuiRenderEngineLowEffect
    return new MiuiRenderEngineLowEffect(density, screenWidth, screenHeight);
}

MiuiRenderEngineLowEffect::MiuiRenderEngineLowEffect (float density, uint32_t screenWidth, uint32_t screenHeight)
        : mVpWidth(screenWidth), mVpHeight(screenHeight){
    (void)(density);
    char value[MPROPERTY_VALUE_MAX];
    __system_property_get("persist.sys.real_blur_radius", value);
    int blurRadius = atoi(value);
    if (blurRadius <= 0) blurRadius = 8;

    __system_property_get("persist.sys.real_blur_black", value);
    int black = atoi(value);
    if (black <= 0) black = 30;
    mBlack = (float)black/100.0f;

    __system_property_get("persist.sys.real_blur_sat", value);
    int increaseSaturation = atoi(value);
    if (increaseSaturation <= 0) increaseSaturation = 20;
    mIncreaseSaturation = (float)increaseSaturation/100.0f;

    mUseHighPrecision = false;
    mMiddleTexturesCount = 0;
    mMiddleTextures = NULL;

    mLayerDrawInfo.mDrawInfo.mBlurTextureWidth = mVpWidth >> SCALE_SHIFT_BIT;
    mLayerDrawInfo.mDrawInfo.mBlurTextureHeight = mVpHeight >> SCALE_SHIFT_BIT;

    for (uint32_t n = mVpHeight/4; n > mLayerDrawInfo.mDrawInfo.mBlurTextureWidth; n /= 4) {
        mMiddleTexturesCount++;
    }

    if (mMiddleTexturesCount > 0) {
        mMiddleTextures = new TextureObject[mMiddleTexturesCount];
        uint32_t currHeight = mVpHeight;
        uint32_t currWidth  = mVpWidth;
        for (uint32_t i = 0; i < mMiddleTexturesCount; i++) {
            currHeight /= 4;
            currWidth  /= 4;
            mMiddleTextures[i].width = currWidth;
            mMiddleTextures[i].height = currHeight;
            createTexture(mMiddleTextures[i]);
        }
    }

    mShaderUtil = MiuiShaderUtilLowEffect(mUseHighPrecision, mBlack, mIncreaseSaturation);

    if (mMiddleTexturesCount > 0) {
        mSimpleProgram = std::make_unique<MiuiProgram>();
        mSimpleProgram->create(mShaderUtil.getVertexShader(), mShaderUtil.simpleFragmentShader());
    }

    mSaturationProgram = std::make_unique<MiuiProgram>();
    mSaturationProgram->create(mShaderUtil.getVertexShader(), mShaderUtil.saturationFragmentShader());

    mBlurXProgram = std::make_unique<MiuiProgram>();
    String8 mblurXProgramString;
    mShaderUtil.getFragmentShader(true, blurRadius, mLayerDrawInfo.mDrawInfo.mBlurTextureWidth, mblurXProgramString);
    mBlurXProgram->create(mShaderUtil.getVertexShader(), mblurXProgramString.c_str());

    mBlurYProgram = std::make_unique<MiuiProgram>();
    String8 mblurYProgramString;
    mShaderUtil.getFragmentShader(false, blurRadius, mLayerDrawInfo.mDrawInfo.mBlurTextureHeight, mblurYProgramString);
    mBlurYProgram->create(mShaderUtil.getVertexShader(), mblurYProgramString.c_str());

    mBlendColorProgram = std::make_unique<MiuiProgram>();
    mBlendColorProgram->create(mShaderUtil.getVertexCode(), mShaderUtil.getBlendColorFragmentCode());

    mRoundCornerProgram = std::make_unique<MiuiProgram>();
    mRoundCornerProgram->create(mShaderUtil.getVertexCode(), mShaderUtil.getRoundCornerCode());

    MMesh::VertexArray<mvec2> position(sSimpleMesh.getPositionArray<mvec2>());
    setupVectors(position, -1.0f, 1.0f, -1.0f, 1.0f);

    MMesh::VertexArray<mvec2> texCoords(sSimpleMesh.getTexCoordArray<mvec2>());
    setupVectors(texCoords, 0.0f, 1.0f, 0.0f, 1.0f);

    prepareViewFunctor();
}

MiuiRenderEngineLowEffect::~MiuiRenderEngineLowEffect() {
    if (mMiddleTextures != NULL) {
        delete [ ] mMiddleTextures;
    }

    if (mWeight != NULL) {
        free(mWeight);
    }
    deletePrograms();
}

} // namespace android
