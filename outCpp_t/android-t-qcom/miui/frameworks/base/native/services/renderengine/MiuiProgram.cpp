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

#include <stdint.h>
#include <GLES2/gl2.h>

#include <android/log.h>
#include "MiuiProgram.h"

namespace android {

MiuiProgram::MiuiProgram() {
    mProgramId = 0;
}

bool MiuiProgram::create(const char* vertexShaderCode, const char* fragmentShaderCode) {
    int vertexShader = 0;
    int fragmentShader = 0;

    vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderCode);
    fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderCode);

    if (vertexShader == 0 || fragmentShader == 0) {
        return false;
    }

    mProgramId = glCreateProgram();
    if (mProgramId != 0) {
        glAttachShader(mProgramId, vertexShader);
        glAttachShader(mProgramId, fragmentShader);

        glBindAttribLocation(mProgramId, position, "a_position");
        glBindAttribLocation(mProgramId, texCoords, "a_texCoords");

        glLinkProgram(mProgramId);

        GLint linkStatus;
        glGetProgramiv(mProgramId, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            glDeleteProgram(mProgramId);
            mProgramId = 0;
            return false;
        }
    }

    glDetachShader(mProgramId, vertexShader);
    glDetachShader(mProgramId, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

GLuint MiuiProgram::loadShader(int type, const char* shaderCode) {
    GLuint shader = glCreateShader(type);
    if (shader != 0) {
        glShaderSource(shader, 1, &shaderCode, 0);
        glCompileShader(shader);

        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_FALSE) {
            GLchar log[512];
            glGetShaderInfoLog(shader, sizeof(log), 0, &log[0]);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

uint32_t MiuiProgram::getProgramId() {
    return mProgramId;
}

void MiuiProgram::deleteProgram() {
    if (mProgramId != 0) {
        glUseProgram(0);
        glDeleteProgram(mProgramId);
    }
}

} // namespace android
