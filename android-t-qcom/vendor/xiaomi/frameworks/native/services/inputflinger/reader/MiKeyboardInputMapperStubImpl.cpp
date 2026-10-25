#include <log/log.h>
#include <cutils/properties.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <android-base/stringprintf.h>
#include <binder/IServiceManager.h>
#include "MiKeyboardInputMapperStubImpl.h"
#include "../knock/InputKnockBase.h"
#include "MiInputManagerStubImpl.h"

namespace android {

MiKeyboardInputMapperStubImpl::MiKeyboardInputMapperStubImpl(){
    ALOGD("%s enter", __func__);
}

MiKeyboardInputMapperStubImpl::~MiKeyboardInputMapperStubImpl(){
    ALOGD("%s enter", __func__);
}

void MiKeyboardInputMapperStubImpl::init(KeyboardInputMapper* keyboardInputMapper) {

}

bool MiKeyboardInputMapperStubImpl::processIntercept(const RawEvent* rawEvent) {
    switch (rawEvent->type) {
        case EV_KEY: {
            int32_t scanCode = rawEvent->code;
            if(scanCode == 0x153) {
                sp<KnockServiceInterface> knockSerivce = MiInputManagerStubImpl::getInstance()->getKnockService();
                if(knockSerivce != nullptr && knockSerivce->isOpenNativeKnock()){
                    knockSerivce->notifyKeyDataReady(rawEvent->value != 0);
                }
                return true;
            } else if (scanCode == 0x152) {
                return true;
            }
        }
    }
    return false;
}

extern "C" IMiKeyboardInputMapperStub* createKeyboardInputMapperStubImpl() {
    return new MiKeyboardInputMapperStubImpl;
}

extern "C" void destroyKeyboardInputMapperStubImpl(IMiKeyboardInputMapperStub* impl) {
    delete impl;
}
}