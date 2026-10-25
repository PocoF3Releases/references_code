/*Gluint
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

#include <vector>
#include "MiuiShaderUtilLowEffect.h"

#define BLUR_MODE_BLACK_SATURATION 0
#define BLUR_MODE_COLOR_DODGE 1
#define BLUR_MODE_WHITE_SATURATION 2
#define BLUR_MODE_COLOR_BURN 3

namespace android {
using namespace std;

template <typename ...Args>
inline string format_string(const char* format, Args... args) {
    constexpr size_t oldlen = BUFSIZ;
    char buffer[oldlen];
    size_t newlen = snprintf(&buffer[0], oldlen, format, args...);
    newlen++;

    if (newlen > oldlen) {
        vector<char> newbuffer(newlen);
        snprintf(newbuffer.data(), newlen, format, args...);
        return string(newbuffer.data());
    }

    return buffer;
}

MiuiShaderUtilLowEffect::MiuiShaderUtilLowEffect(bool useHighPrecision, int black, int increaseSaturation) {
    mUseHighPrecision = useHighPrecision;
    mBlack = black;
    mIncreaseSaturation = increaseSaturation;
}

const char* MiuiShaderUtilLowEffect::getVertexShader() {
    return "attribute vec2 a_position;\n"
            "attribute vec2 a_texCoords;\n"
            "varying vec2 texCoords;\n"
            "void main(void) {\n"
            "    texCoords = a_texCoords;\n"
            "    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);\n"
            "}\n";
}

const char* MiuiShaderUtilLowEffect::simpleFragmentShader() {
    return "precision mediump float;\n"
           "uniform sampler2D sampler1;\n"
           "varying vec2 texCoords;\n"
           "void main(void) {\n"
           "    gl_FragColor = texture2D(sampler1, texCoords);\n"
           "}\n";
}

const char* MiuiShaderUtilLowEffect::saturationFragmentShader() {
    return  "precision mediump float;\n"
            "uniform sampler2D sampler1;\n"
            "varying vec2 texCoords;\n"
            "uniform float increaseSaturation;\n"
            "uniform float black;\n"
            "uniform int mode;\n"
            "void main(void) {\n"
            "    vec3 colorOri = texture2D(sampler1, texCoords).xyz;\n"
            "    float opaque = 1.0;\n"
            "    float dodge = 0.75;\n"
            "    float burn = 0.25;\n"
            "    if (mode == 0) {\n"
            "        vec3 colorIncrease = vec3("
            "             colorOri.r * (1.0 + 2.0742 * increaseSaturation) + colorOri.g * (-1.8282 * increaseSaturation) + colorOri.b * (-0.246 * increaseSaturation),"
            "             colorOri.r * (-0.9258 * increaseSaturation) + colorOri.g * (1.0 + 1.1718 * increaseSaturation) + colorOri.b * (-0.246 * increaseSaturation),"
            "             colorOri.r * (-0.9258 * increaseSaturation) + colorOri.g * (-1.8282 * increaseSaturation) + colorOri.b * (1.0 + 2.754 * increaseSaturation));\n"
            "        gl_FragColor = vec4(colorIncrease * (1.0-black), opaque);\n"
            "    } else if (mode == 1) {\n"
            "        vec3 colorDodge = vec3(dodge, dodge, dodge);\n"
            "        vec3 colorWhite = vec3(opaque, opaque, opaque);\n"
            "        gl_FragColor = vec4(colorDodge * 0.1 + colorWhite * 0.4 + colorOri * 0.5, 1.0);\n"
            "    } else if (mode == 2) {\n"
            "        vec3 colorIncrease = vec3("
            "        colorOri.r * (1.0 + 2.0742 * increaseSaturation) + colorOri.g * (-1.8282 * increaseSaturation) + colorOri.b * (-0.246 * increaseSaturation),"
            "        colorOri.r * (-0.9258 * increaseSaturation) + colorOri.g * (1.0 + 1.1718 * increaseSaturation) + colorOri.b * (-0.246 * increaseSaturation),"
            "        colorOri.r * (-0.9258 * increaseSaturation) + colorOri.g * (-1.8282 * increaseSaturation) + colorOri.b * (1.0 + 2.754 * increaseSaturation));\n"
            "        vec3 colorWhite = vec3(opaque, opaque, opaque);\n"
            "        gl_FragColor = vec4(colorIncrease * (1.0-black) + colorWhite * black, opaque);\n"
            "    } else if (mode == 3) {\n"
            "        vec3 colorBurn = vec3(burn, burn, burn);\n"
            "        gl_FragColor = vec4(colorOri * 0.5 + colorBurn * 0.1, opaque);\n"
            "    } else {\n"
            "        gl_FragColor = vec4(colorOri, opaque);\n"
            "    }\n"
            "}\n" ;
}

void MiuiShaderUtilLowEffect::getFragmentShader(bool isX, int radius, int textureSize, String8 &builder) {
    if (radius%2 == 1) radius += 1;

    float step = 1.0/textureSize;

    int factorAmount = 0;
    for (int i = 1; i <= radius; ++i) {
        factorAmount += i;
    }
    factorAmount = (factorAmount * 2) + (radius + 1);
    float factor = 1.0/factorAmount;

    builder.clear();

    if (mUseHighPrecision) {
        builder +=
                "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
                "precision highp float;\n"
                "#else\n"
                "precision mediump float;\n"
                "#endif\n";
    } else {
        builder += "precision mediump float;\n";
    }

    builder +=
            "uniform sampler2D sampler;\n"
            "varying vec2 texCoords;\n";

    builder.appendFormat(
            "void main(void) {\n"
            "vec3 sumRgb = texture2D(sampler, vec2(texCoords.x, texCoords.y)).xyz * %10.9f;\n",
        (radius + 1) * factor);

    for (int n = 0; n < 2; ++n) {
        for (int i = 1; i < radius; i += 2) {
            int weight = (radius - i) * 2 + 1;
            float distance = step * (i + ((float)(radius - i))/weight);
            builder.appendFormat(
                isX ?
                    "sumRgb += texture2D(sampler, vec2(texCoords.x %s %10.9f, texCoords.y)).xyz * %10.9f;\n":
                    "sumRgb += texture2D(sampler, vec2(texCoords.x, texCoords.y %s %10.9f)).xyz * %10.9f;\n",
                n == 0 ? "+" : "-",
                distance,
                weight * factor);
        }
    }

    builder +=
        "gl_FragColor = vec4(sumRgb, 1.0);\n"
        "}\n";
}

const char* MiuiShaderUtilLowEffect::getVertexCode() {
    return "uniform mat4 uMVPMatrix;\n"
           "uniform mat4 uTexMatrix;\n"
           "attribute vec2 aTexCoord;\n"
           "attribute vec3 aPosition;\n"
           "varying vec2 vTexCoord;\n"
           "void main() {\n"
           "    gl_Position = uMVPMatrix * vec4(aPosition, 1);\n"
           "    vTexCoord = (uTexMatrix * vec4(aTexCoord, 1, 1)).st;\n"
           "}\n";
}

const char* MiuiShaderUtilLowEffect::getFragmentShaderCode() {
    return "\n"
           "precision mediump float;\n"
           "varying vec2 vTexCoord;\n"
           "uniform sampler2D uTexture;\n"
           "uniform int uRadius;\n"
           "uniform float uWidthOffset;\n"
           "uniform float uHeightOffset;\n"
           "mediump float getGaussWeight(mediump float currentPos, mediump float sigma)\n"
           "{\n"
           "    return 1.0 / sigma * exp(-(currentPos * currentPos) / (2.0 * sigma * sigma));\n"
           "}\n"
           "void main() {\n"
           "    int diameter = 2 * uRadius + 1;\n"
           "    vec4 sampleTex;\n"
           "    vec3 col;\n"
           "    float weightSum = 0.0;\n"
           "    for(int i = 0; i < diameter; i++) {\n"
           "        vec2 offset = vec2(float(i - uRadius) * uWidthOffset, float(i - uRadius) * uHeightOffset);\n"
           "        sampleTex = vec4(texture2D(uTexture, vTexCoord.st+offset));\n"
           "        float index = float(i);\n"
           "        float gaussWeight = getGaussWeight(index - float(diameter - 1)/2.0,"
           "        (float(diameter - 1)/2.0 + 1.0) / 2.0);\n"
           "        col += sampleTex.rgb * gaussWeight;\n"
           "        weightSum += gaussWeight;\n"
           "    }\n"
           "    gl_FragColor = vec4(col / weightSum, sampleTex.a);\n"
           "}\n";
}

/**
* If set kernel weight array in advance, the GPU registers have no enough space.
* So compute the weight in the code directly.
*/
const char* MiuiShaderUtilLowEffect::getGaussianSampleCode() {
    return "int diameter = 2 * uRadius + 1;\n"
           "vec4 sampleTex;\n"
           "vec3 col;\n"
           "float weightSum = 0.0;\n"
           "for(int i = 0; i < diameter; i++) {\n"
           "   vec2 offset = vec2(float(i - uRadius) * uWidthOffset, float(i - uRadius) * uHeightOffset);\n"
           "   sampleTex = vec4(texture2D(uTexture, vTexCoord.st+offset));\n"
           "   float index = float(i);\n"
           "   float gaussWeight = getGaussWeight(index - float(diameter - 1)/2.0,"
           "   (float(diameter - 1)/2.0 + 1.0) / 2.0);\n"
           "   col += sampleTex.rgb * gaussWeight;\n"
           "   weightSum += gaussWeight;\n"
           "}\n"
           "gl_FragColor = vec4(col / weightSum, sampleTex.a);\n";
}

/**
* If set kernel weight array in advance, the GPU registers have no enough space.
* So compute the weight in the code directly.
*/
const char* MiuiShaderUtilLowEffect::getBoxSampleCode() {
    return "   int diameter = 2 * uRadius + 1; \n"
           "   vec4 sampleTex;\n"
           "   vec3 col;  \n"
           "   float weightSum = 0.0; \n"
           "   for(int i = 0; i < diameter; i++) {\n"
           "       vec2 offset = vec2(float(i - uRadius) * uWidthOffset, float(i - uRadius) * uHeightOffset);  \n"
           "        sampleTex = vec4(texture2D(uTexture, vTexCoord.st+offset));\n"
           "       float index = float(i); \n"
           "       float boxWeight = float(1.0) / float(diameter); \n"
           "       col += sampleTex.rgb * boxWeight; \n"
           "       weightSum += boxWeight;\n"
           "   }   \n"
           "   gl_FragColor = vec4(col / weightSum, sampleTex.a);   \n";
}

/**
* If set kernel weight array in advance, the GPU registers have no enough space.
* So compute the weight in the code directly.
*/
const char* MiuiShaderUtilLowEffect::getStackSampleCode() {
    return "int diameter = 2 * uRadius + 1;  \n"
           "   vec4 sampleTex;\n"
           "   vec3 col;  \n"
           "   float weightSum = 0.0; \n"
           "   for(int i = 0; i < diameter; i++) {\n"
           "       vec2 offset = vec2(float(i - uRadius) * uWidthOffset, float(i - uRadius) * uHeightOffset);  \n"
           "       sampleTex = vec4(texture2D(uTexture, vTexCoord.st+offset));\n"
           "       float index = float(i); \n"
           "       float boxWeight = float(uRadius) + 1.0 - abs(index - float(uRadius)); \n"
           "       col += sampleTex.rgb * boxWeight; \n"
           "       weightSum += boxWeight;\n"
           "   }   \n"
           "   gl_FragColor = vec4(col / weightSum, sampleTex.a);   \n";
}
const char* MiuiShaderUtilLowEffect::getBlendColorFragmentCode() {
       return " \n"
              "precision mediump float;\n"
              "varying vec2 vTexCoord;\n"
              "uniform sampler2D uTexture;\n"
              "uniform lowp float mixPercent;\n"
              "uniform vec4 vMixColor;\n"
              "uniform int vMixMode;\n"

              "float blendColorDodge(float dst, float src, float dstAlpha, float srcAlpha) {\n"
              " return (src == 1.0) ? src : min(dst/(1.0-src),1.0);   \n"
              "}\n"

              "float blendOverlay(float dst, float src, float dstAlpha, float srcAlpha) {\n"
              "   return dst < 0.5 * dstAlpha ? (2.0 * src * dst) : ((srcAlpha * dstAlpha) - 2.0 * (dstAlpha - src) * (srcAlpha - dst));\n"
              "}\n"

              "float blendColorBurn(float dst, float src, float dstAlpha, float srcAlpha) {\n"
              "   return (src == 0.0) ? src : max((1.0 - ((1.0-dst) / src)),0.0);\n"
              "}\n"

              "float blendHardLight(float dst, float src, float dstAlpha, float srcAlpha) {   \n"
              " if (src * (1.0 - dstAlpha) + dst * (1.0 - srcAlpha) + 2.0 * src <= srcAlpha) {   \n"
              "   return 2.0 * src * dst;\n"
              " } else {\n"
              "   return srcAlpha * dstAlpha - 2.0 * (dstAlpha - dst) * (srcAlpha - src);\n"
              " }\n"
              "}\n"

              "float blendSoftLight(float dst, float src, float dstAlpha, float srcAlpha) {\n"
              "   float m;\n"
              "   float g;\n"
              "   float f;\n"
              "   if(dstAlpha > 0.0) {\n"
              "       m = dst / dstAlpha;\n"
              "   } else {\n"
              "       m = 0.0;\n"
              "   }\n"
              "   if(4.0 * dst <= dstAlpha) {\n"
              "       g = (16.0 * m * m + 4.0 * m) * (m - 1.0) + 7.0 * m;\n"
              "   } else {\n"
              "       g = sqrt(m) - m;\n"
              "   }\n"
              "   if(2.0 * src <= srcAlpha) {\n"
              "       f = dst * (srcAlpha + (2.0 * src - srcAlpha) * (1.0 - m));\n"
              "   } else {\n"
              "       f = dst * srcAlpha + dstAlpha * (2.0 * src - srcAlpha) * g;\n"
              "   }\n"
              "   return f + (src/srcAlpha) + (dst/dstAlpha);\n"
              "}\n"

              "vec3 RGBToHSL(vec3 color) {\n"
              "   vec3 hsl;\n"
              "   float fmin = min(min(color.r, color.g), color.b);\n"
              "   float fmax = max(max(color.r, color.g), color.b);\n"
              "   float delta = fmax - fmin;\n"
              "   hsl.z = (fmax + fmin) / 2.0;\n"
              "   if (delta == 0.0) {\n"
              "       hsl.x = 0.0;\n"
              "       hsl.y = 0.0;\n"
              "   } else {\n"
              "       if (hsl.z < 0.5)\n"
              "           hsl.y = delta / (fmax + fmin);\n"
              "       else\n"
              "           hsl.y = delta / (2.0 - fmax - fmin);\n"
              "       float deltaR = (((fmax - color.r) / 6.0) + (delta / 2.0)) / delta;\n"
              "       float deltaG = (((fmax - color.g) / 6.0) + (delta / 2.0)) / delta;\n"
              "       float deltaB = (((fmax - color.b) / 6.0) + (delta / 2.0)) / delta;\n"
              "       if (color.r == fmax)\n"
              "           hsl.x = deltaB - deltaG;\n"
              "       else if (color.g == fmax)\n"
              "           hsl.x = (1.0 / 3.0) + deltaR - deltaB;\n"
              "       else if (color.b == fmax)\n"
              "           hsl.x = (2.0 / 3.0) + deltaG - deltaR;\n"
              "       if (hsl.x < 0.0)\n"
              "           hsl.x += 1.0;\n"
              "       else if (hsl.x > 1.0)\n"
              "           hsl.x -= 1.0;\n"
              "   }\n"
              "   return hsl;\n"
              "}\n"


              "float HueToRGB(float f1, float f2, float hue) {\n"
              "   if (hue < 0.0)\n"
              "       hue += 1.0;\n"
              "   else if (hue > 1.0)\n"
              "       hue -= 1.0;\n"
              "   float res;\n"
              "   if ((6.0 * hue) < 1.0)\n"
              "       res = f1 + (f2 - f1) * 6.0 * hue;\n"
              "   else if ((2.0 * hue) < 1.0)\n"
              "       res = f2;\n"
              "   else if ((3.0 * hue) < 2.0)\n"
              "       res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;\n"
              "   else\n"
              "       res = f1;\n"
              "   return res;\n"
              "}\n"

              "vec3 HSLToRGB(vec3 hsl) {\n"
              "   vec3 rgb;\n"
              "   if (hsl.y == 0.0) {\n"
              "       rgb = vec3(hsl.z);\n"
              "   } else {\n"
              "       float f2;\n"
              "       if (hsl.z < 0.5)\n"
              "           f2 = hsl.z * (1.0 + hsl.y);\n"
              "       else\n"
              "           f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);\n"
              "       float f1 = 2.0 * hsl.z - f2;\n"
              "       rgb.r = HueToRGB(f1, f2, hsl.x + (1.0/3.0));\n"
              "       rgb.g = HueToRGB(f1, f2, hsl.x);\n\n"
              "       rgb.b= HueToRGB(f1, f2, hsl.x - (1.0/3.0));\n"
              "   }\n"
              "   return rgb;\n"
              "}\n"

              "void main() {   \n"
              "   vec4 col = vec4(texture2D(uTexture, vTexCoord.st));\n"
              "   float radiuAlpha = 1.0;   \n"
              "   if (vMixMode == 0) {   \n"
              "     gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);   \n"
              "   } else if (vMixMode == 1) {   \n"
              "     float result = vMixColor.a * radiuAlpha;\n"
              "     gl_FragColor = vec4(vMixColor.rgb, result);\n"
              "   } else if (vMixMode == 2) {   \n"
              "     float result = col.a * radiuAlpha;\n"
              "     gl_FragColor = vec4(col.rgb, result);\n"
              "   } else if (vMixMode == 3) {   \n"
              "     float alpha = vMixColor.a + (1.0 - vMixColor.a) * col.a;\n"
              "     vec3 colResult = vMixColor.rgb + (1.0 - vMixColor.a) * col.rgb;\n"
              "     float result = alpha * radiuAlpha;\n"
              "     gl_FragColor = vec4(colResult, result);\n"
              "   } else if (vMixMode == 4) {   \n"
              "     float alpha = col.a + (1.0 - col.a) * vMixColor.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = col.rgb + (1.0 - col.a) * vMixColor.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 5) {   \n"
              "     float alpha = vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vMixColor.rgb * col.a;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 6) {   \n"
              "     float alpha = vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = col.rgb * vMixColor.a;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 7) {   \n"
              "     float alpha = (1.0 - col.a) * vMixColor.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = (1.0 - col.a) * vMixColor.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 8) {   \n"
              "     float alpha = (1.0 - vMixColor.a) * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = (1.0 - vMixColor.a) * col.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 9) {   \n"
              "     float alpha = col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = col.a * vMixColor.rgb + (1.0 - vMixColor.a) * col.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 10) {   \n"
              "     float alpha = vMixColor.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vMixColor.a * col.rgb + (1.0 - col.a) * vMixColor.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 11) {   \n"
              "     float alpha = (1.0 - col.a) * vMixColor.a + (1.0 - vMixColor.a) * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = (1.0 - col.a) * vMixColor.rgb + (1.0 - vMixColor.a) * col.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 12) {   \n"
              "     float alpha = max(0.0, min(vMixColor.a + col.a, 1.0));\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = max(vec3(0.0), min(vMixColor.rgb + col.rgb, vec3(1.0)));\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 13) {   \n"
              "     float alpha = vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vMixColor.rgb * col.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 14) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vMixColor.rgb + col.rgb - vMixColor.rgb * col.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 15) {   \n"
              "     float alpha = col.a + vMixColor.a - col.a * vMixColor.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vec3(blendOverlay(col.r,vMixColor.r, col.a, vMixColor.a),blendOverlay(col.g, vMixColor.g, col.a, vMixColor.a),blendOverlay(col.b,vMixColor.b, col.a, vMixColor.a));\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 16) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = (1.0 - col.a) * vMixColor.rgb + (1.0 - vMixColor.a) * col.rgb + min(vMixColor.rgb, col.rgb);\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 17) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = (1.0 - col.a) * vMixColor.rgb + (1.0 - vMixColor.a) * col.rgb + max(vMixColor.rgb, col.rgb);\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 18) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vec3(blendColorDodge(col.r,vMixColor.r, col.a, vMixColor.a),blendColorDodge(col.g,vMixColor.g, col.a, vMixColor.a),blendColorDodge(col.b,vMixColor.b, col.a, vMixColor.a));\n"
              "     colResult = colResult * vMixColor.a + vec3(col.r, col.g, col.b) * (1.0 - vMixColor.a);\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 19) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vec3(blendColorBurn(col.r, vMixColor.r, col.a, vMixColor.a), blendColorBurn(col.g, vMixColor.g, col.a, vMixColor.a), blendColorBurn(col.b, vMixColor.b, col.a, vMixColor.a));\n"
              "     colResult = colResult * vMixColor.a + vec3(col.r, col.g, col.b) * (1.0 - vMixColor.a);\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 20) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vec3(blendHardLight(col.r,vMixColor.r, col.a, vMixColor.a),blendHardLight(col.g,vMixColor.g, col.a, vMixColor.a),blendHardLight(col.b,vMixColor.b, col.a, vMixColor.a));\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 21) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vec3(blendSoftLight(col.r,vMixColor.r, col.a, vMixColor.a),blendSoftLight(col.g,vMixColor.g, col.a, vMixColor.a),blendSoftLight(col.b,vMixColor.b, col.a, vMixColor.a));\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 22) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vMixColor.rgb + col.rgb - 2.0 * min(vMixColor.rgb * col.a, col.rgb * vMixColor.a);\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 23) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vMixColor.rgb + col.rgb - 2.0 * vMixColor.rgb * col.rgb ;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 24) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = vMixColor.rgb * (1.0 - col.a) + col.rgb * (1.0 - vMixColor.a) + (vMixColor.rgb * col.rgb);\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 25) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colHSL = RGBToHSL(col.rgb);   \n"
              "     vec3 colResult = HSLToRGB(vec3(RGBToHSL(vMixColor.rgb).r, colHSL.g, colHSL.b));\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 26) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colHSL = RGBToHSL(col.rgb);   \n"
              "     vec3 colResult = HSLToRGB(vec3(colHSL.r, RGBToHSL(vMixColor.rgb).g, colHSL.b));\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 27) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 mixHSL = RGBToHSL(vMixColor.rgb);   \n"
              "     vec3 colResult = HSLToRGB(vec3(mixHSL.r, mixHSL.g, RGBToHSL(col.rgb).b));\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 28) {   \n"
              "     float alpha = vMixColor.a + col.a - vMixColor.a * col.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colHSL = RGBToHSL(col.rgb);   \n"
              "     vec3 colResult = HSLToRGB(vec3(colHSL.r, colHSL.g, RGBToHSL(vMixColor.rgb).b));\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else if (vMixMode == 29) { \n"
              "     float alpha = col.a + (1.0 - col.a) * vMixColor.a;\n"
              "     float result = alpha * radiuAlpha;   \n"
              "     vec3 colResult = (1.0 - vMixColor.a ) * col.rgb + vMixColor.a * vMixColor.rgb;\n"
              "     gl_FragColor = vec4(colResult, result);   \n"
              "   } else {   \n"
              "     float result = col.a * radiuAlpha;   \n"
              "     gl_FragColor = vec4(col.rgb, result);   \n"
              "   }\n"
              "}   \n";
}

const char* MiuiShaderUtilLowEffect::getRoundCornerCode() {
       return " \n"
              "precision mediump float;\n"
              "varying vec2 vTexCoord;\n"
              "uniform sampler2D uTexture;\n"

              "uniform float rtCornerRadius;\n"
              "uniform float rbCornerRadius;\n"
              "uniform float ltCornerRadius;\n"
              "uniform float lbCornerRadius;\n"
              "uniform vec2 rtCornerCenter;\n"
              "uniform vec2 rbCornerCenter;\n"
              "uniform vec2 lbCornerCenter;\n"
              "uniform vec2 ltCornerCenter;\n"
              "uniform vec2 cropSize;   \n"

              "float applyCornerRadius(vec2 cropCoords) {   \n"
              "   float dist = 0.0; \n"
              "   vec2 center = vec2(0.0);\n"
              "   float rtcDist = 0.0; \n"
              "   float rbcDist = 0.0; \n"
              "   float lbcDist = 0.0; \n"
              "   float ltcDist = 0.0; \n"
              "   vec2 rtCenter = vec2(0.0);\n"
              "   vec2 rbCenter = vec2(0.0);\n"
              "   vec2 lbCenter = vec2(0.0);\n"
              "   vec2 ltCenter = vec2(0.0);\n"

              "   vec2 RTC = vec2(1.0);\n"
              "   vec2 RBC = vec2(1.0, 0.0);\n"
              "   vec2 LBC = vec2(0.0, 0.0);\n"
              "   vec2 LTC = vec2(0.0, 1.0);\n"

              "   float sfx0 = 1.0;\n"
              "   float sfx1 = 1.0;\n"
              "   float sfy0 = 1.0;\n"
              "   float sfy1 = 1.0;\n"

              "   if(cropSize.x > cropSize.y) { \n"
              "       sfx0=cropSize.y/cropSize.x;\n"
              "       sfx1=cropSize.y/pow(cropSize.x,2.0);\n"
              "   } else {\n"
              "       sfy0=cropSize.x/cropSize.y;\n"
              "       sfy1=cropSize.x/pow(cropSize.y,2.0);\n"
              "   }\n"

              "   rtCenter = rtCornerCenter;\n"
              "   if (rtCenter.x < cropCoords.x && rtCenter.y < cropCoords.y) {\n"
              "       rtcDist = sqrt(pow(rtCenter.x - cropCoords.x, 2.0)/sfx1 + pow(rtCenter.y - cropCoords.y, 2.0)/sfy1);\n"
              "   }\n"

              "   ltCenter = ltCornerCenter;\n"
              "   if (ltCenter.x > cropCoords.x && ltCenter.y < cropCoords.y) {\n"
              "       ltcDist = sqrt(pow(ltCenter.x - cropCoords.x, 2.0)/sfx1 + pow(ltCenter.y - cropCoords.y, 2.0)/sfy1);\n"
              "   }\n"

              "   lbCenter = lbCornerCenter;\n"
              "   if (lbCenter.x > cropCoords.x && lbCenter.y > cropCoords.y) {\n"
              "       lbcDist = sqrt(pow(lbCenter.x - cropCoords.x, 2.0)/sfx1 + pow(lbCenter.y - cropCoords.y, 2.0)/sfy1);\n"
              "   }\n"

              "   rbCenter = rbCornerCenter;\n"
              "   if (rbCenter.x < cropCoords.x && rbCenter.y > cropCoords.y) {\n"
              "       rbcDist = sqrt(pow(rbCenter.x - cropCoords.x, 2.0)/sfx1 + pow(rbCenter.y - cropCoords.y, 2.0)/sfy1);\n"
              "   }\n"

              "   if(rbcDist > rbCornerRadius || lbcDist > lbCornerRadius || ltcDist > ltCornerRadius || rtcDist > rtCornerRadius) {   \n"
              "       return 0.0;   \n"
              "   } else {   \n"
              "       return 1.0;   \n"
              "   }   \n"
              "}   \n"

              "void main() {   \n"
              "   vec4 col = vec4(texture2D(uTexture, vTexCoord.st));\n"
              "   float radiuAlpha = applyCornerRadius(vTexCoord.st);   \n"

              "   float result = col.a * radiuAlpha;   \n"
              "   gl_FragColor = vec4(col.rgb, result);   \n"
              "}   \n";
}
}
