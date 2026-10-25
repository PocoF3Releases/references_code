#include "MiGLStubImpl.h"
#include "migl.h"
#include "hooks/migl_modify_params.h"

using namespace android::migl;

namespace android {
    MiGLStubImpl impl;

    #ifdef __cplusplus
    extern "C" {
    #endif /* __cplusplus */

    IMiGLStub* createInstance() {
        createThreadSpecificKey();
        initGameSetting();
        return &impl;
    }

    void destroyInstance(IMiGLStub* impl) {
        // deleteThreadSpecificKey();
    }

    #ifdef __cplusplus
    }
    #endif /* __cplusplus */
}
