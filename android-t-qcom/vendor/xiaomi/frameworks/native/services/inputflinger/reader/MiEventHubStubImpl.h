#ifndef ANDROID_MI_EVENTHUB_IMPL_H
#define ANDROID_MI_EVENTHUB_IMPL_H

#include <android-base/thread_annotations.h>
#include <cinttypes>
#include <InputListener.h>
#include "IMiEventHubStub.h"
#include "EventHub.h"
#include "miinput/MiInputManager.h"

namespace android {

class MiEventHubStubImpl : public IMiEventHubStub {
public:
    MiEventHubStubImpl();
    void init(EventHub* eventHub) override;
    void onConfigurationChanged(InputConfigTypeEnum type, int32_t changes);
    std::function<void(InputConfigTypeEnum, int32_t)> mOnConfigurationChangedListener =
            [this](InputConfigTypeEnum typeEnum, int32_t changes)
            {onConfigurationChanged(typeEnum, changes); };
protected:
    virtual ~MiEventHubStubImpl();
private:
    EventHub *mEventHub = nullptr;
    static MiEventHubStubImpl* sImplInstance;
    MiInputManager* mMiInputManager = nullptr;
    static std::mutex mInstLock;
};

} //namespace:android
#endif