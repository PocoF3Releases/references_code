#ifndef ANDROID_MI_CURSORINPUTMAPPER_IMPL_H
#define ANDROID_MI_CURSORINPUTMAPPER_IMPL_H

#include <android-base/thread_annotations.h>
#include <cinttypes>
#include "IMiCursorInputMapperStub.h"
#include "CursorInputMapper.h"
#include "miinput/MiInputManager.h"

namespace android {

class MiCursorInputMapperStubImpl : public IMiCursorInputMapperStub {
public:
    MiCursorInputMapperStubImpl();
    void init() override;
    void sync(float* vscroll, float* hscroll) override;
    void onConfigurationChanged(InputConfigTypeEnum type, int32_t changes);
    std::function<void(InputConfigTypeEnum, int32_t)> mOnConfigurationChangedListener =
            [this](InputConfigTypeEnum typeEnum, int32_t changes)
            {onConfigurationChanged(typeEnum, changes); };
protected:
    virtual ~MiCursorInputMapperStubImpl();
private:
    MiInputManager* mMiInputManager = nullptr;
    bool mMouseNaturalScroll = false;
};

} //namespace:android
#endif