#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <log/log.h>
#include "HandyModeUtils.h"

namespace android {

bool HandyModeUtils::mHasLoadedTextures = false;

unsigned char* HandyModeUtils::readPNG(const char* file, png_uint_32* w, png_uint_32* h)
{
    FILE* f;
    unsigned char sig[8];
    png_structp png_ptr;
    png_infop   info_ptr;
    int bit_depth;
    int color_type;
    unsigned int rowbytes;
    unsigned char* image_data;
    png_uint_32 i;
    png_bytepp row_pointers;

    if ((f = fopen(file, "rb")) == NULL) {
        return nullptr;
    }
    fread(sig, sizeof(*sig), sizeof(sig), f);
    if (!png_check_sig(sig, sizeof(*sig))) {
        fclose(f);
        return nullptr;
    }
    if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) {
        fclose(f);
        return nullptr;
    }
    if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(f);
        return nullptr;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(f);
        return nullptr;
    }
    png_init_io(png_ptr, f);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, w, h, &bit_depth, &color_type, NULL, NULL, NULL);
    if (bit_depth > 8)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    if ((image_data =(unsigned char *) malloc(*h * rowbytes)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(f);
        return nullptr;
    }
    if ((row_pointers =(png_bytepp) malloc(*h * sizeof(png_bytep))) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(f);
        free(image_data);
        return nullptr;
    }

    for (i = 0; i < *h; i++) {
        row_pointers[i] = image_data + i*rowbytes;
    }

    png_read_image(png_ptr, row_pointers);

    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(f);

    return image_data;
}

void HandyModeUtils::destroyTextures(renderengine::Texture* titleTexture,
                                     renderengine::Texture* backgroundTexture,
                                     renderengine::Texture* settingIconTexture) {
    if (titleTexture != nullptr) {
        GLuint texName = titleTexture->getTextureName();
        glDeleteFramebuffers(1, &texName);
        delete titleTexture;
        titleTexture = nullptr;
    }
    if (backgroundTexture != nullptr) {
        GLuint texName = backgroundTexture->getTextureName();
        glDeleteFramebuffers(1, &texName);
        delete backgroundTexture;
        backgroundTexture = nullptr;
    }
    if (settingIconTexture != nullptr) {
        GLuint texName = settingIconTexture->getTextureName();
        glDeleteFramebuffers(1, &texName);
        delete settingIconTexture;
        settingIconTexture = nullptr;
    }
    mHasLoadedTextures = false;
}

//bool HandyModeUtils::hasLoadedTextures() {
//    return mHasLoadedTextures;
//}

void HandyModeUtils::loadTextures(renderengine::Texture** titleTexture,
                                  renderengine::Texture** backgroundTexture,
                                  renderengine::Texture** settingIconTexture) {
    *titleTexture = loadTexture("/data/system/title_image_for_handymode.png", true);
    *backgroundTexture = loadTexture("/data/system/blured_wallpaper.png", true);
    *settingIconTexture = loadTexture("/data/system/setting_icon_for_handymode.png", true);
    mHasLoadedTextures = true;
}

renderengine::Texture* HandyModeUtils::loadTexture(const char* rawImagePath, bool filter) {
    // load bitmap data
    png_uint_32 width;
    png_uint_32 height;
    unsigned char* buffer = nullptr;
    buffer = readPNG(rawImagePath, &width, &height);
    if (buffer == nullptr) return nullptr;

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
}

} /* namespace android */
