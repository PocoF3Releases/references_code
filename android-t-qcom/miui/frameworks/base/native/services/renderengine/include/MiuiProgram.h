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

#ifndef MIUI_PROGRAM_H
#define MIUI_PROGRAM_H

#include <stdint.h>

namespace android {

/*
 * Abstracts a GLSL program comprising a vertex and fragment shader
 */
class MiuiProgram {
public:
    enum {
        /* Crop coordinates, in pixels */
        cropCoords = 0,
        /* position of each vertex for vertex shader */
        position = 1,
        /* UV coordinates for texture mapping */
        texCoords = 2,
    };

    MiuiProgram();
    bool create(const char* vertexShaderCode, const char* fragmentShaderCode);
    ~MiuiProgram() = default;
    void deleteProgram();
    uint32_t getProgramId();

private:
    uint32_t loadShader(int type, const char* shaderCode);
    // Name of the OpenGL program and shaders
    uint32_t mProgramId;
};
} // namespace android

#endif /* SF_BLUR_PROGRAM_H */
