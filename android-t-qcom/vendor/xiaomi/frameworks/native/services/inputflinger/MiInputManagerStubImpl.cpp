#define LOG_TAG "MiInputManagerStubImpl"

#include <log/log.h>
#include <cutils/properties.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <android-base/stringprintf.h>
#include <dlfcn.h>
#include <binder/IServiceManager.h>
#include "MiInputManagerStubImpl.h"
#include "reader/MiuiInputMapper.h"

namespace android {
MiInputManagerStubImpl* MiInputManagerStubImpl::sImplInstance = NULL;
std::mutex MiInputManagerStubImpl::mInstLock;

MiInputManagerStubImpl::MiInputManagerStubImpl(){
    ALOGD("%s enter", __func__);
}

MiInputManagerStubImpl* MiInputManagerStubImpl::getInstance() {
    std::lock_guard<std::mutex> lock(mInstLock);
    if (!sImplInstance) {
        sImplInstance = new MiInputManagerStubImpl();
    }
    return sImplInstance;
}

sp<KnockServiceInterface>  MiInputManagerStubImpl::getKnockService() {
    return mKnockService;
}

MiInputManagerStubImpl::~MiInputManagerStubImpl(){
    ALOGD("%s enter", __func__);
}

MiuiInputMapper* MiInputManagerStubImpl::getMiuiInputMapper() {
    return mMiuiInputMapper;
}

void MiInputManagerStubImpl::init(InputManager* InputManager) {
    this->mInputManager = InputManager;
}

InputListenerInterface* MiInputManagerStubImpl::createMiInputMapper(InputListenerInterface& listener) {
    mMiuiInputMapper = new MiuiInputMapper(listener);
#ifdef MIUI_BUILD
    mKnockService = createKnockService(mMiuiInputMapper);
#endif
    return mMiuiInputMapper;
}

sp<KnockServiceInterface> MiInputManagerStubImpl::createKnockService(InputListenerInterface* listener) {
    sp<KnockServiceInterface> knockService = nullptr;
    void * LibKnockServiceImpl;
    LibKnockServiceImpl = dlopen(LIBPATH, RTLD_LAZY);
    if (LibKnockServiceImpl) {
        typedef KnockServiceInterface* Create(InputListenerInterface* listener);
        Create* create = (Create*)dlsym(LibKnockServiceImpl, "createKnockService");
        if (create) {
            knockService = create(listener);
        } else {
            ALOGE("dlsym can't find createKnockService,with error:%s", dlerror());
        }
    } else {
        ALOGE("dlopen %s failed,with error:%s", LIBPATH, dlerror());
    }
    return knockService;
}

status_t MiInputManagerStubImpl::start() {
    if (mKnockService) {
        return mKnockService->start();
    }
    return 0;
}

status_t MiInputManagerStubImpl::stop() {
    if (mKnockService) {
        return mKnockService->stop();
    }
    return 0;
}

void MiInputManagerStubImpl::dump(std::string& dumpStr) {
    dumpStr += "MiInputManagerStubImpl State:\n";
    MiInputManager::getInstance()->dumpMiInputManager(dumpStr);
    if (mKnockService) {
        mKnockService->dump(dumpStr);
    }
}

extern "C" IMiInputManagerStub* createInputManagerStubImpl() {
    return MiInputManagerStubImpl::getInstance();
}

extern "C" void destroyInputManagerStubImpl(IMiInputManagerStub* impl) {
    delete impl;
}
}