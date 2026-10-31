#define LOG_TAG "MiInputManager"

#include "MiInputManager.h"
#include "MiInputManagerStubImpl.h"

namespace android {

MiInputManager* MiInputManager::sImplInstance = NULL;
std::mutex MiInputManager::mInstLock;
std::mutex MiInputManager::mIistenerLock;

MiInputManager::MiInputManager() {
    mInputDebugConfig = new InputDebugConfig(false, false, false, false, false);
    mInputCommonConfig = new InputCommonConfig(0, 0);
    mInputKnockConfig = new InputKnockConfig();
}

MiInputManager* MiInputManager::getInstance() {
    std::lock_guard<std::mutex> lock(mInstLock);
    if (!sImplInstance) {
        sImplInstance = new MiInputManager();
    }
    return sImplInstance;
}

int32_t MiInputManager::setInputConfig(int32_t configType, Parcel* parcel) {
    if (parcel == nullptr || configType <= 0) {
        ALOGE("native set input_config error, configType: %d, parcel:%s ", configType,
              parcel ? "not null" : "null");
        return INPUT_CONFIG_TYPE_ERROR;
    }
    status_t status = INPUT_CONFIG_TYPE_ERROR;
    InputConfigTypeEnum type = INPUT_CONFIG_TYPE_ERROR;
    if (configType == INPUT_CONFIG_TYPE_DEBUG_CONFIG) {
        type = INPUT_CONFIG_TYPE_DEBUG_CONFIG;
        status = InputDebugConfig::readFromParcel(*parcel, mInputDebugConfig);
    } else if (configType == INPUT_CONFIG_TYPE_COMMON_CONFIG) {
        type = INPUT_CONFIG_TYPE_COMMON_CONFIG;
        status = InputCommonConfig::readFromParcel(*parcel, mInputCommonConfig);
    } else if (configType == INPUT_CONFIG_TYPE_KNOCK_CONFIG) {
        type = INPUT_CONFIG_TYPE_KNOCK_CONFIG;
        status = InputKnockConfig::readFromParcel(*parcel, mInputKnockConfig);
    }
    if (status > 0) {
        onConfigurationChanged(type, status);
    }

    ALOGE("setInputConfig not match config, configType: %d, parcel:%s ", configType,
          parcel ? "not null" : "null");
    return status;
}

void MiInputManager::addOnConfigurationChangedListener(
        const std::function<void(InputConfigTypeEnum, int32_t)>& onConfigurationChangedListener) {
    std::lock_guard<std::mutex> lock(mIistenerLock);
    mOnConfigurationChangedListeners.push_back(onConfigurationChangedListener);
}

void MiInputManager::onConfigurationChanged(InputConfigTypeEnum type, int32_t changes) {
    std::lock_guard<std::mutex> lock(mIistenerLock);
    for (const auto& item : mOnConfigurationChangedListeners) {
        item(type, changes);
    }
}

void MiInputManager::injectMotionEvent(const MotionEvent* event, int mode) {
    MiInputManagerStubImpl::getInstance()->getMiuiInputMapper()->injectMotionEvent(event, mode);
}

bool MiInputManager::getInputDispatcherMajor() {
    return mInputDebugConfig->mInputDispatcherMajor;
}

bool MiInputManager::getInputDispatcherDetail() {
    return mInputDebugConfig->mInputDispatcherDetail;
}
bool MiInputManager::getInputDispatcherAll() {
    return mInputDebugConfig->mInputDispatcherAll;
}

bool MiInputManager::getInputReaderAll() {
    return mInputDebugConfig->mInputReaderAll;
}
bool MiInputManager::getInputTransportAll() {
    return mInputDebugConfig->mInputTransportAll;
}

bool MiInputManager::getSynergyMode() {
    return mInputCommonConfig->mSynergyMode;
}

bool MiInputManager::isInputMethodShown() {
    return mInputCommonConfig->mInputMethodShown;
}

bool MiInputManager::isCustomizeInputMethod() {
    return mInputCommonConfig->mCustomizeInputMethod;
}

bool MiInputManager::isPadMode() {
    return mInputCommonConfig->mPadMode;
}

bool MiInputManager::isOnewayMode() {
    return mInputCommonConfig->mOnewayMode;
}

void MiInputManager::dumpMiInputManager(std::string& dump) {
    std::scoped_lock _l(mInstLock);
    mInputDebugConfig->dump(dump);
    mInputCommonConfig->dump(dump);
    mInputKnockConfig->dump(dump);
}

} // namespace android