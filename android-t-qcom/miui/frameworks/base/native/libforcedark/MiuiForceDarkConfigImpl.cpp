#undef LOG_TAG
#define LOG_TAG "MiuiForceDarkConfig"

#include "MiuiForceDarkConfigImpl.h"
#include <android-base/stringprintf.h>

using namespace android;

MiuiForceDarkConfigImpl* MiuiForceDarkConfigImpl::sImplInstance = NULL;
std::mutex MiuiForceDarkConfigImpl::mInstLock;

MiuiForceDarkConfigImpl* MiuiForceDarkConfigImpl::getInstance() {
    std::lock_guard<std::mutex> lock(mInstLock);
    if (!sImplInstance) {
        sImplInstance = new MiuiForceDarkConfigImpl();
    }
    return sImplInstance;
}

void MiuiForceDarkConfigImpl::setConfig(float density, int mainRule,
                                        int secondaryRule, int tertiaryRule){
    ALOGI("setConfig density:%f, mainRule:%d, secondaryRule:%d, tertiaryRule:%d"
          , density, mainRule, secondaryRule, tertiaryRule);
    mDensity = density;
    mMainRule = mainRule;
    mSecondaryRule = secondaryRule;
    mTertiaryRule = tertiaryRule;
}

float MiuiForceDarkConfigImpl::getDensity(){
    return mDensity;
}

int MiuiForceDarkConfigImpl::getMainRule() {
    return mMainRule;
}

int MiuiForceDarkConfigImpl::getSecondaryRule() {
    return mSecondaryRule;
}

int MiuiForceDarkConfigImpl::getTertiaryRule() {
    return mTertiaryRule;
}

extern "C" IMiuiForceDarkConfigStub* createForceDark() {
    return MiuiForceDarkConfigImpl::getInstance();
}

extern "C" void destroyForceDark(IMiuiForceDarkConfigStub* impl) {
    delete impl;
}
