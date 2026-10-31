#ifndef ANDROID_MI_INPUTREADER_IMPL_H
#define ANDROID_MI_INPUTREADER_IMPL_H
//because we can't require mLock (in dispatcher private member), we disable thread-safety-analysis.
#pragma clang diagnostic ignored "-Wthread-safety-analysis"

#include <android-base/thread_annotations.h>
#include <cinttypes>
#include <InputListener.h>
#include <PointerControllerInterface.h>
#include "IMiInputReaderStub.h"
#include "InputReader.h"
#include "InputDevice.h"
#include "miinput/MiInputManager.h"
#include "../dispatcher/MiInputDispatcherStubImpl.h"

namespace android {
class InputDevice;

class MiInputReaderStubImpl : public IMiInputReaderStub {
public:
    MiInputReaderStubImpl();
    void init(InputReader* inputReader) override;
    bool getInputReaderAll() override;
    void addDeviceLocked(std::shared_ptr<InputDevice> inputDevice) override;
    void removeDeviceLocked(std::shared_ptr<InputDevice> inputDevice) override;
    bool handleConfigurationChangedLockedIntercept(nsecs_t when) override;
    void loopOnceChanges(uint32_t changes) override;
    void onConfigurationChanged(InputConfigTypeEnum type, int32_t changes);
    std::function<void(InputConfigTypeEnum, int32_t)> mOnConfigurationChangedListener =
            [this](InputConfigTypeEnum typeEnum, int32_t changes)
            {onConfigurationChanged(typeEnum, changes); };

protected:
    virtual ~MiInputReaderStubImpl();
private:
    bool isXiaomiKeyboardChanged;
    bool mSystemReady;
    InputReader* mInputReader;
    MiInputManager* mMiInputManager = nullptr;
    //Default value is K81's Keyboard Info
    uint16_t mMiuiKeyboardVendorId = 0x3206;
    uint16_t mMiuiKeyboardProductId = 0x3ffc;
    static std::mutex mInstLock;
    static MiInputReaderStubImpl* sImplInstance;
    const ui::Transform getTransformForDisplay(int displayId) const;
public:
    static MiInputReaderStubImpl* getInstance();
    bool setCursorPosition(float x, float y);
    bool hideCursor();
};

} //namespace:android
#endif