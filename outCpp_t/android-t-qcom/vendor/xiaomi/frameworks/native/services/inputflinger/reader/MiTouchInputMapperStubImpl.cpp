#include <log/log.h>
#include <cutils/properties.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <android-base/stringprintf.h>
#include <binder/IServiceManager.h>
#include "MiTouchInputMapperStubImpl.h"
#include "../knock/InputKnockBase.h"
#include "MiInputManagerStubImpl.h"

namespace android {

MiTouchInputMapperStubImpl::MiTouchInputMapperStubImpl(){
    ALOGD("%s enter", __func__);
}

MiTouchInputMapperStubImpl::~MiTouchInputMapperStubImpl(){
    ALOGD("%s enter", __func__);
}

void MiTouchInputMapperStubImpl::init(TouchInputMapper* touchInputMapper) {

}

bool MiTouchInputMapperStubImpl::dispatchMotionIntercept(NotifyMotionArgs* args, int surfaceOrientation) {
    sp<KnockServiceInterface> knockSerivce = MiInputManagerStubImpl::getInstance()->getKnockService();
    if(knockSerivce != nullptr && knockSerivce->isOpenNativeKnock()){
        if(args->pointerProperties[0].toolType == AMOTION_EVENT_TOOL_TYPE_FINGER) {
            return knockSerivce->isNeedSaveMotion(args, surfaceOrientation);
        }
    }
    return false;
}

extern "C" IMiTouchInputMapperStub* createTouchInputMapper() {
    return new MiTouchInputMapperStubImpl;
}

extern "C" void destroyTouchInputMapper(IMiTouchInputMapperStub* impl) {
    delete impl;
}
}