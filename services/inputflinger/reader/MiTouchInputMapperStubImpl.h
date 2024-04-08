#ifndef ANDROID_MI_TOUCHINPUTMAPPER_IMPL_H
#define ANDROID_MI_TOUCHINPUTMAPPER_IMPL_H

#include <android-base/thread_annotations.h>
#include <cinttypes>
#include <InputListener.h>
#include "IMiTouchInputMapperStub.h"
#include "TouchInputMapper.h"

namespace android {

class MiTouchInputMapperStubImpl : public IMiTouchInputMapperStub {
public:
    MiTouchInputMapperStubImpl();
    void init(TouchInputMapper* touchInputMapper) override;
    bool dispatchMotionIntercept(NotifyMotionArgs* args, int surfaceOrientation) override;

protected:
    virtual ~MiTouchInputMapperStubImpl();
private:
};

} //namespace:android
#endif