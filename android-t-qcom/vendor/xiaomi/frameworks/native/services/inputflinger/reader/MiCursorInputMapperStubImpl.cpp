#include <log/log.h>
#include <cutils/properties.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <android-base/stringprintf.h>
#include <binder/IServiceManager.h>
#include "MiCursorInputMapperStubImpl.h"

namespace android {

MiCursorInputMapperStubImpl::MiCursorInputMapperStubImpl(){
    mMiInputManager = MiInputManager::getInstance();
    MiInputManager::getInstance()->addOnConfigurationChangedListener(
            mOnConfigurationChangedListener);
    ALOGD("%s enter", __func__);
}

MiCursorInputMapperStubImpl::~MiCursorInputMapperStubImpl(){
    ALOGD("%s enter", __func__);
}

void MiCursorInputMapperStubImpl::init() {
    mMouseNaturalScroll = mMiInputManager->mInputCommonConfig->mMouseNaturalScroll;
}

void MiCursorInputMapperStubImpl::sync(float* vscroll, float* hscroll) {
    if (mMouseNaturalScroll) {
        *vscroll = -*vscroll;
        *hscroll = -*hscroll;
    }
}

void MiCursorInputMapperStubImpl::onConfigurationChanged(InputConfigTypeEnum type, int32_t changes) {
    if(type == INPUT_CONFIG_TYPE_COMMON_CONFIG){
        if((changes & CONFIG_MOUSE_NATURAL_SCROLL) != 0) {
            mMouseNaturalScroll = mMiInputManager->mInputCommonConfig->mMouseNaturalScroll;
        }
    }
}

extern "C" IMiCursorInputMapperStub* createCursorInputMapperStubImpl() {
    return new MiCursorInputMapperStubImpl;
}

extern "C" void destroyCursorInputMapperStubImpl(IMiCursorInputMapperStub* impl) {
    delete impl;
}
}