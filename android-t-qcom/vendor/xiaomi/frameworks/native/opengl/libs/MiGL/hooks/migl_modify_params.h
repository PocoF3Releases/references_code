#ifndef __MIGL_MODIFY_PARAMES_H__
#define __MIGL_MODIFY_PARAMES_H__

#include "hooks.h"

namespace android {

void initGameSetting();
void modifyRenderPrecisionYuanshen(GLsizei& width, GLsizei& height, int api);
void modifyAFValuePubg(GLenum pname, GLint& param);
void modifyShadingRate();
void updateTexSizeArr();
void checkBossPattern();
void updateEleCountArr(const GLsizei& count);
void modifyRenderPrecisionYuanshenShaderUniform(GLsizei count, GLfloat *value);
}

#endif
