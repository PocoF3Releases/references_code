#ifndef ANDROID_MI_KEYBOARDINPUTMAPPER_IMPL_H
#define ANDROID_MI_KEYBOARDINPUTMAPPER_IMPL_H

#include <android-base/thread_annotations.h>
#include <cinttypes>
#include <InputListener.h>
#include "IMiKeyboardInputMapperStub.h"
#include "KeyboardInputMapper.h"

namespace android {

class MiKeyboardInputMapperStubImpl : public IMiKeyboardInputMapperStub {
public:
    MiKeyboardInputMapperStubImpl();
    void init(KeyboardInputMapper* keyboardInputMapper) override;
    bool processIntercept(const RawEvent* rawEvent) override;
    
protected:
    virtual ~MiKeyboardInputMapperStubImpl();
private:
};

} //namespace:android
#endif