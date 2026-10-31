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

#ifndef MIUI_SHADER_UTIL_H
#define MIUI_SHADER_UTIL_H

#include <stdint.h>
#include <string.h>
#include <string>
#include <sstream>

namespace android {

using namespace std;
/*
* Abstracts a GLSL program comprising a vertex and fragment shader
*/
class MiuiShaderUtil {
public:
    enum {
        /* Crop coordinates, in pixels */
        MODE_BOX = 0,
        /* position of each vertex for vertex shader */
        MODE_GAUSSIAN = 1,
        /* UV coordinates for texture mapping */
        MODE_STACK = 2,
    };

    enum {
        BLEND_MODE_DEFAULT = -1,
        BLEND_MODE_CLEAR = 0,
        BLEND_MODE_SRC = 1,
        BLEND_MODE_DST = 2,
        BLEND_MODE_SRC_OVER = 3,
        BLEND_MODE_DST_OVER = 4,
        BLEND_MODE_SRC_IN = 5,
        BLEND_MODE_DST_IN = 6,
        BLEND_MODE_SRC_OUT = 7,
        BLEND_MODE_DST_OUT = 8,
        BLEND_MODE_SRC_ATOP = 9,
        BLEND_MODE_DST_ATOP = 10,
        BLEND_MODE_XOR = 11,
        BLEND_MODE_PLUS = 12,
        BLEND_MODE_MODULATE = 13,
        BLEND_MODE_SCREEN = 14,
        BLEND_MODE_OVERLAY = 15,
        BLEND_MODE_DARKEN = 16,
        BLEND_MODE_LIGHTEN = 17,
        BLEND_MODE_COLOR_DODGE = 18,
        BLEND_MODE_COLOR_BURN = 19,
        BLEND_MODE_HARD_LIGHT = 20,
        BLEND_MODE_SOFT_LIGHT = 21,
        BLEND_MODE_DIFFERENCE = 22,
        BLEND_MODE_EXCLUSION = 23,
        BLEND_MODE_MULTIPLY = 24,
        BLEND_MODE_HUE = 25,
        BLEND_MODE_SATURATION = 26,
        BLEND_MODE_COLOR = 27,
        BLEND_MODE_LUMINOSITY = 28,
    };

    bool mUseHighPrecision;
    int mMaxBlurRadius;

    MiuiShaderUtil() { };
    MiuiShaderUtil(bool useHighPrecision, int maxBlurRadius);
    ~MiuiShaderUtil() = default;

    // from surfaceflinger miuiblur shader
    const char* getVertexShader();
    const char* simpleFragmentShader();
    const char* saturationFragmentShader();
    void getFragmentShader(bool isX, string &builder);

    // from blurdrawable sdk shader
    const char* getVertexCode();
    const char* getFragmentShaderCode();
    const char* getGaussianSampleCode();
    const char* getBoxSampleCode();
    const char* getStackSampleCode();
    const char* getCopyFragmentCode();
    const char *getRoundCornerCode();
    const char *getBlendColorFragmentCode();
};
} // namespace android

#endif /* SF_BLUR_PROGRAM_H */

