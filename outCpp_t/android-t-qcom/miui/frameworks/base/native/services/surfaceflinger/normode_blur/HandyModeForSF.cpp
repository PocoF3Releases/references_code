#include <cmath>
#include <stdio.h>
#include <GLES2/gl2.h>
#include <png.h>
#ifdef SDK_GREATER_THAN_KITKAT
#include <cutils/compiler.h>
#endif
#include "HandyModeForSF.h"

namespace android {

static float interpolator(float r) {
    r = r - 1.0f;
    return (float)sqrt(1.0f - r * r);
}

static float calcScale(int animateState, float animateCurrRate, float finalScale) {
    const float scaleRate = animateState == ANIMATE_STATE_MOVING ? 1.0f : animateCurrRate;
    return 1.0f - scaleRate * (1.0f - finalScale);
}

static float calcPivotX(int animateState, float animateCurrRate, int newMode, float screenWidth) {
    if (animateState == ANIMATE_STATE_MOVING) {
        return (screenWidth * (newMode == MODE_LEFT ? 1.0f - animateCurrRate : animateCurrRate));
    } else {
        return (newMode == MODE_LEFT ? 0 : screenWidth);
    }
}

static void setupVectors(renderengine::Mesh::VertexArray<vec2>& vectors, float x1, float x2, float y1, float y2) {
    vectors[0] = vec2(x1, y1);
    vectors[1] = vec2(x1, y2);
    vectors[2] = vec2(x2, y2);
    vectors[3] = vec2(x2, y1);
}

/* static renderengine::Texture* loadTexture(const char* rawImagePath, bool filter) {
    // load bitmap data
    png_uint_32 width;
    png_uint_32 height;
    unsigned char* buffer = nullptr;
    HandyModeUtils::readPNG(rawImagePath, &width, &height, buffer);
    if (buffer == NULL) return NULL;

    // build texture from bitmap data
    uint32_t texName;
    glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_2D, texName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    free(buffer);
    buffer = nullptr;
    renderengine::Texture* texture = new renderengine::Texture(renderengine::Texture::TEXTURE_2D, texName);
    texture->setFiltering(filter);
    texture->setDimensions((size_t)width, (size_t)height);
    return texture;
} */

HandyModeForSF* HandyModeForSF::sInstance = NULL;

HandyModeForSF::HandyModeForSF()
    : mSimpleMesh(renderengine::Mesh::Builder()
                        .setPrimitive(renderengine::Mesh::TRIANGLE_FAN)
                        .setVertices(4 /* count */, 2 /* size */)
                        .setTexCoords(2 /* size */)
                        .setCropCoords(2 /* size */)
                        .build()) {
    mMode = MODE_NONE;
    mScale = 1.0f;
    mScreenWidth = 0;
    mScreenHeight = 0;
    mBackMode = MODE_NONE;
    mAnimateState = ANIMATE_STATE_CLOSED;
    mAnimateCurrRate = 0;
    mTexturesIsLoaded = false;
    mTitleTexture = nullptr;
    mBackgroundTexture = nullptr;
    mSettingIconMargin = 0;
    mScaleforHandyMode = 0.0;
}

HandyModeForSF::~HandyModeForSF() {
}

void HandyModeForSF::init() {
    if (sInstance != NULL) return;
    sInstance = new HandyModeForSF();
}

void HandyModeForSF::setupScreenSize(int w, int h) {
    mScreenWidth = w > h ? h : w;
    mScreenHeight = w > h ? w : h;
    renderengine::Mesh::VertexArray<vec2> position(mSimpleMesh.getPositionArray<vec2>());
    setupVectors(position, 0.0f, mScreenWidth, mScreenHeight, 0.0f);
    renderengine::Mesh::VertexArray<vec2> texCoords(mSimpleMesh.getTexCoordArray<vec2>());
    setupVectors(texCoords, 0.0f, 1.0f, 0.0f, 1.0f);
}

/* void HandyModeForSF::loadTextures() {
    mTitleTexture = loadTexture("/data/system/title_image_for_handymode.png", true);
    mBackgroundTexture = loadTexture("/data/system/blured_wallpaper.png", true);
    mSettingIconTexture = loadTexture("/data/system/setting_icon_for_handymode.png", true);
    mSettingIconMargin = (int32_t)((mScreenWidth - mScreenWidth * mScale - mSettingIconTexture->getWidth()) / 2);
} */

void HandyModeForSF::destroyTextures() {
    if (mTitleTexture != nullptr) {
        GLuint texName = mTitleTexture->getTextureName();
        glDeleteFramebuffers(1, &texName);
        delete mTitleTexture;
        mTitleTexture = nullptr;
    }
    if (mBackgroundTexture != nullptr) {
        GLuint texName = mBackgroundTexture->getTextureName();
        glDeleteFramebuffers(1, &texName);
        delete mBackgroundTexture;
        mBackgroundTexture = nullptr;
    }
    if (mSettingIconTexture != nullptr) {
        GLuint texName = mSettingIconTexture->getTextureName();
        glDeleteFramebuffers(1, &texName);
        delete mSettingIconTexture;
        mSettingIconTexture = nullptr;
    }
}

void HandyModeForSF::onDoComposeSurfaces(renderengine::RenderEngine* renderEngine) {
    if (isInHandyMode()) {
        mSettingIconMargin = (int32_t)((mScreenWidth - mScreenWidth * mScale - mSettingIconTexture->getWidth()) / 2);

        // draw background
        if (mBackgroundTexture != nullptr) {
            renderEngine->setupLayerTexturing(*mBackgroundTexture);
            //dont disable texture
            renderEngine->setupLayerBlending(true, true,false, 255, 0);
            renderEngine->drawMesh(mSimpleMesh);
            renderEngine->disableTexturing();
        } else {
            renderEngine->clearWithColor(0.0f, 0.0f, 0.0f, 0.0f);
        }

        // draw title
        if (mTitleTexture != nullptr) {
            int verticalTotalPadding = mScreenHeight * (1.0f - mScale) - mTitleTexture->getHeight();
            if (verticalTotalPadding >= 0) {
                int horizontalPadding = (mScreenWidth - mTitleTexture->getWidth()) / 2;
                int topPadding = (int) (mScreenHeight * 0.058f);
                if (topPadding > verticalTotalPadding / 2) {
                    topPadding = verticalTotalPadding / 2;
                }
        renderengine::Mesh mesh(renderengine::Mesh::Builder()
                        .setPrimitive(renderengine::Mesh::TRIANGLE_FAN)
                        .setVertices(4 /* count */, 2 /* size */)
                        .setTexCoords(2 /* size */)
                        .setCropCoords(2 /* size */)
                        .build());
        renderengine::Mesh::VertexArray<vec2> texCoords(mesh.getTexCoordArray<vec2>());
        setupVectors(texCoords, 0.0f, 1.0f, 0.0f, 1.0f);
        renderengine::Mesh::VertexArray<vec2> position(mesh.getPositionArray<vec2>());
        setupVectors(position, horizontalPadding, mTitleTexture->getWidth() + horizontalPadding,
                    (mScreenHeight - (mScreenHeight - topPadding)),
                    mScreenHeight - (mScreenHeight - topPadding - mTitleTexture->getHeight()));

        renderEngine->setupLayerTexturing(*mTitleTexture);

               //dont disable texture
            renderEngine->setupLayerBlending(false, false,false, 255, 0);
            renderEngine->drawMesh(mesh);
            renderEngine->disableTexturing();
            }
        }

        // draw setting icon
        if (mSettingIconTexture != nullptr && mSettingIconMargin >= 0) {
            //dont disable texture
            renderEngine->setupLayerBlending(false, false,false, 255, 0);

            renderengine::Mesh mesh(renderengine::Mesh::Builder()
                        .setPrimitive(renderengine::Mesh::TRIANGLE_FAN)
                        .setVertices(4 /* count */, 2 /* size */)
                        .setTexCoords(2 /* size */)
                        .setCropCoords(2 /* size */)
                        .build());
            renderengine::Mesh::VertexArray<vec2> position(mesh.getPositionArray<vec2>());
            renderengine::Mesh::VertexArray<vec2> texCoords(mesh.getTexCoordArray<vec2>());

            if (mMode == MODE_RIGHT || mAnimateState == ANIMATE_STATE_MOVING) {
                setupVectors(texCoords, 0.0f, 1.0f, 0.0f, 1.0f);
                int right = mSettingIconMargin + mSettingIconTexture->getWidth();

                setupVectors(position, mSettingIconMargin, right,
                    (mScreenHeight - (mSettingIconMargin + mSettingIconTexture->getHeight())), (mScreenHeight - mSettingIconMargin));
                renderEngine->setupLayerTexturing(*mSettingIconTexture);
                renderEngine->drawMesh(mesh);
            }

            if (mMode == MODE_LEFT || mAnimateState == ANIMATE_STATE_MOVING) {
                setupVectors(texCoords, 1.0f, 0.0f, 0.0f, 1.0f);
                int left = mScreenWidth - mSettingIconMargin - mSettingIconTexture->getWidth();
                int right = mScreenWidth - mSettingIconMargin;

                setupVectors(position, left, right,
                    (mScreenHeight - (mSettingIconMargin + mSettingIconTexture->getHeight())), (mScreenHeight - mSettingIconMargin));
                renderEngine->setupLayerTexturing(*mSettingIconTexture);
                renderEngine->drawMesh(mesh);
            }

            renderEngine->disableTexturing();
        }

        // refresh animate state
        if (mAnimateState == ANIMATE_STATE_OPENING
                || mAnimateState == ANIMATE_STATE_MOVING
                || mAnimateState == ANIMATE_STATE_CLOSING) {
            float rate = (float)((systemTime()/1000000 - mAnimateStartTime))/200;

            if (rate > 1.0f) rate = 1.0f;
            else rate = interpolator(rate);

            if (mAnimateState == ANIMATE_STATE_CLOSING) {
                rate = 1.0f - rate;
            }

            if (rate == 1.0f
                    && (mAnimateState == ANIMATE_STATE_OPENING || mAnimateState == ANIMATE_STATE_MOVING)) {
                mAnimateState = ANIMATE_STATE_OPENED;
            } else if (rate == 0.0f && mAnimateState == ANIMATE_STATE_CLOSING) {
                mAnimateState = ANIMATE_STATE_CLOSED;
                mMode = MODE_NONE;
                mTransform.reset();
            } else {
                mReCallBack->repaintCallback();
            }
            mAnimateCurrRate = rate;
        }

        // destroy background if quited
        if (mMode == MODE_NONE && mTexturesIsLoaded) {
            destroyTextures();
            mTexturesIsLoaded = false;
        }
    }
}

void HandyModeForSF::onPreLayerDraw() {
    if (mMode != MODE_NONE) {
        const float scale = calcScale(mAnimateState, mAnimateCurrRate, mScale);
        const float xPivot = calcPivotX(mAnimateState, mAnimateCurrRate, mMode, mScreenWidth);
        const float yPivot = mScreenHeight;

        int x = 0;
        int y = 0;
        int w;
        int h;
        if (mMode == MODE_LEFT || mMode == MODE_RIGHT) {
            w = (int)(mScreenWidth*scale);
            h = (int)(mScreenHeight*scale);
            x = (int)processCoordinate(0, 0, scale, xPivot);
            y = (int)processCoordinate(0, 0, scale, yPivot);
        } else {
            w = (int)mScreenWidth;
            h = mScreenHeight - mScreenHeight/2 * mAnimateCurrRate;
        }
        glScissor(x, y, w, h);
        glEnable(GL_SCISSOR_TEST);
    }
}

void HandyModeForSF::onPostLayerDraw() {
    glDisable(GL_SCISSOR_TEST);
}

void HandyModeForSF::onPreScreenshotRender() {
    mBackMode = mMode;
    mMode = MODE_NONE;
}

void HandyModeForSF::onPostScreenshotRender() {
    mMode = mBackMode;
}

void HandyModeForSF::onPostLayerComputeGeometry(renderengine::Mesh& mesh) {
    if (isInHandyMode()) {
        renderengine::Mesh::VertexArray<vec2> position(mesh.getPositionArray<vec2>());

        if (mMode == MODE_DOWN) {
            const float offsetY = mScreenHeight / 2 * mAnimateCurrRate;
            for (size_t i=0 ; i<4 ; i++) {
                vec2& pos = position[i];
                pos.y = processCoordinate(pos.y, offsetY, 1.0f, 0);
            }
        } else {
            const float scale = calcScale(mAnimateState, mAnimateCurrRate, mScale);
            mScaleforHandyMode = scale;
            const float xPivot = calcPivotX(mAnimateState, mAnimateCurrRate, mMode, mScreenWidth);
            const float yPivot = mScreenHeight;
            // Set translate x and y
            mTransform.set(processCoordinate(0, 0, scale, xPivot), processCoordinate(0, 0, scale, yPivot));
            // Set scale
            mTransform.set(scale, 0, 0, scale);
            for (size_t i=0 ; i<4 ; i++) {
                vec2& pos = position[i];
                pos.x = processCoordinate(pos.x, 0, scale, xPivot);
                pos.y = processCoordinate(pos.y, 0, scale, yPivot);
            }
        }
    }
}

void HandyModeForSF::onPartBlurScissor(vec2& pos, details::TVec2<int>& size) {
    if (isInHandyMode()) {
        if (mMode == MODE_DOWN) {
            const float offsetY = mScreenHeight / 2 * mAnimateCurrRate;
            pos.y = processCoordinate(pos.y, offsetY, 1.0f, 0);
        } else {
            const float scale = calcScale(mAnimateState, mAnimateCurrRate, mScale);
            const float xPivot = calcPivotX(mAnimateState, mAnimateCurrRate, mMode, mScreenWidth);
            pos.x = processCoordinate(pos.x, 0, scale, xPivot);
            pos.y = processCoordinate(pos.y, 0, scale, 0);
            size.x *= scale;
            size.y *= scale;
        }
    }
}

} /* namespace android */
