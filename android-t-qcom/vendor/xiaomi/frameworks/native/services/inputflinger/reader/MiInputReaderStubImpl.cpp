#include <log/log.h>
#include <cutils/properties.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <android-base/stringprintf.h>
#include <binder/IServiceManager.h>
#include "MiInputReaderStubImpl.h"
#include "MiInputManager.h"
#include "MiEventHubStubImpl.h"

namespace android {

MiInputReaderStubImpl* MiInputReaderStubImpl::sImplInstance = NULL;
std::mutex MiInputReaderStubImpl::mInstLock;
const ui::Transform kIdentityTransform;

MiInputReaderStubImpl::MiInputReaderStubImpl(){
    isXiaomiKeyboardChanged = false;
    mSystemReady = false;
    mMiInputManager = MiInputManager::getInstance();
    MiInputManager::getInstance()->addOnConfigurationChangedListener(
            mOnConfigurationChangedListener);
    ALOGD("%s enter", __func__);
}

MiInputReaderStubImpl::~MiInputReaderStubImpl(){
    ALOGD("%s enter", __func__);
}

MiInputReaderStubImpl* MiInputReaderStubImpl::getInstance() {
    std::lock_guard<std::mutex> lock(mInstLock);
    if (!sImplInstance) {
        sImplInstance = new MiInputReaderStubImpl();
    }
    return sImplInstance;
}

void MiInputReaderStubImpl::init(InputReader* inputReader) {
    this->mInputReader = inputReader;
}

bool MiInputReaderStubImpl::getInputReaderAll() {
    return MiInputManager::getInstance()->getInputReaderAll();
}

void MiInputReaderStubImpl::addDeviceLocked(std::shared_ptr<InputDevice> device) {
    if (device->getProduct() == mMiuiKeyboardProductId &&
          device->getVendor() == mMiuiKeyboardVendorId) {
        isXiaomiKeyboardChanged = true;
    }
}

void MiInputReaderStubImpl::removeDeviceLocked(std::shared_ptr<InputDevice> device) {
    if (device->getProduct() == mMiuiKeyboardProductId &&
          device->getVendor() == mMiuiKeyboardVendorId) {
        isXiaomiKeyboardChanged = true;
    }
}

bool MiInputReaderStubImpl::handleConfigurationChangedLockedIntercept(nsecs_t when) {
    bool isPadMode = MiInputManager::getInstance()->isPadMode();
    if(isPadMode && isXiaomiKeyboardChanged) {
        return true;
    }
    return false;
}

void MiInputReaderStubImpl::loopOnceChanges(uint32_t changes) {
    if(changes & InputReaderConfiguration::CHANGE_DEVICE_ALIAS && !mSystemReady){
        mSystemReady = true;
        mInputReader->bumpGenerationLockedForStub();
    }
}

void MiInputReaderStubImpl::onConfigurationChanged(InputConfigTypeEnum type, int32_t changes) {
    if (type == INPUT_CONFIG_TYPE_COMMON_CONFIG) {
        if ((changes & CONFIG_MIUI_KEYBOARD_INFO) != 0) {
            //Device info
            mMiuiKeyboardVendorId = mMiInputManager->mInputCommonConfig->mMiuiKeyboardVendorId;
            mMiuiKeyboardProductId = mMiInputManager->mInputCommonConfig->mMiuiKeyboardProductId;
        }
    }
}

bool MiInputReaderStubImpl::setCursorPosition(float x, float y) {
    std::shared_ptr<PointerControllerInterface> controller =
            this->mInputReader->getPointerControllerForStub().lock();
    if (controller == nullptr) {
        return false;
    }
    int32_t displayId = controller->getDisplayId();
    const auto transform = getTransformForDisplay(displayId);
    const auto xy = transform.inverse().transform(x, y);
    controller->setPosition(xy.x, xy.y);
    return true;
}

bool MiInputReaderStubImpl::hideCursor() {
    std::shared_ptr<PointerControllerInterface> controller =
            this->mInputReader->getPointerControllerForStub().lock();
    if (controller == nullptr) {
        return false;
    }
    controller->fade(PointerControllerInterface::Transition::IMMEDIATE);
    return true;
}

const ui::Transform MiInputReaderStubImpl::getTransformForDisplay(int displayId) const {
    ui::Transform transform = kIdentityTransform;
    const auto& di = MiInputDispatcherStubImpl::getInstance()->getDisplayInfos();
    if (const auto it = di.find(displayId); it != di.end()) {
        transform = it->second.transform;
    }
    return transform;
}

extern "C" IMiInputReaderStub* createInputReaderStubImpl() {
    return MiInputReaderStubImpl::getInstance();
}

extern "C" void destroyInputReaderStubImpl(IMiInputReaderStub* impl) {
    delete impl;
}
}