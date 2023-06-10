#include <log/log.h>
#include <cutils/properties.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <android-base/stringprintf.h>
#include <binder/IServiceManager.h>
#include "MiEventHubStubImpl.h"
#include "miinput/MiInputManager.h"

namespace android {

MiEventHubStubImpl* MiEventHubStubImpl::sImplInstance = NULL;
std::mutex MiEventHubStubImpl::mInstLock;

MiEventHubStubImpl::MiEventHubStubImpl(){
    mMiInputManager = MiInputManager::getInstance();
    MiInputManager::getInstance()->addOnConfigurationChangedListener(
            mOnConfigurationChangedListener);
    ALOGD("%s enter", __func__);
}

MiEventHubStubImpl::~MiEventHubStubImpl(){
    ALOGD("%s enter", __func__);
}

void MiEventHubStubImpl::init(EventHub* eventHub) {
    this->mEventHub = eventHub;
}

void MiEventHubStubImpl::onConfigurationChanged(InputConfigTypeEnum type, int32_t changes) {
    if (type == INPUT_CONFIG_TYPE_COMMON_CONFIG) {
        if((changes & CONFIG_TOUCH_WAKEUP) != 0) {
            mEventHub->switchTouchWorkMode(mMiInputManager->mInputCommonConfig->mTouchWakeupMode);
        }
    }
}

extern "C" IMiEventHubStub* createEventHubStubImpl() {
    return new MiEventHubStubImpl;
}

extern "C" void destroyEventHubStubImpl(IMiEventHubStub* impl) {
    delete impl;
}
}