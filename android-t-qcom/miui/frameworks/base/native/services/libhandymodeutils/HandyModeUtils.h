#ifndef HANDY_MODE_UTILS_H
#define HANDY_MODE_UTILS_H

#include <GLES2/gl2.h>
#include <png.h>
#include "../../../../../../frameworks/native/libs/renderengine/include/renderengine/Texture.h"

namespace android {

class HandyModeUtils
{

public:
    static renderengine::Texture* loadTexture(const char* rawImagePath, bool filter);
    static unsigned char* readPNG(const char* file, png_uint_32* w, png_uint_32* h);
    static void loadTextures(renderengine::Texture** titleTexture,
                             renderengine::Texture** backgroundTexture,
                             renderengine::Texture** settingIconTexture);
    static void destroyTextures(renderengine::Texture* titleTexture,
                                renderengine::Texture* backgroundTexture,
                                renderengine::Texture* settingIconTexture);
    static bool hasLoadedTextures(){return mHasLoadedTextures;};

private:
    static bool mHasLoadedTextures;

};

}

#endif /* HANDY_MODE_UTILS_H */
