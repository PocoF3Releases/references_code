#ifndef MIUI_BLUR_DRAWFUNCTOR_H
#define MIUI_BLUR_DRAWFUNCTOR_H

#include <android/log.h>

#include "MMesh.h"
#include <utils/Functor.h>
#include "MiuiRenderEngine.h"

namespace android {
    class BlurDrawableFunctor : public Functor {
    public:
        mutable mutex mStateLock;
        FunctorData mFunctor;
        MiuiRenderEngine* mBlurRender = nullptr;
        BlurDrawableFunctor();
        ~BlurDrawableFunctor();
        void renderFrame(DrawGlInfo *c_drawGlInfo);

        status_t operator()(int mode, void *info);
    private:
        void operate(int mode, void *info);
    };
}
#endif
