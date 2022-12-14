#ifndef ANDROID_MIUI_FORTH_DARK_IMPL_H
#define ANDROID_MIUI_FORTH_DARK_IMPL_H

#include "hwui/IMiuiForceDarkConfigStub.h"

#include <utils/RefBase.h>
#include <utils/threads.h>
#include <limits.h>
#include <utils/Mutex.h>
#include <utils/Looper.h>
#include <utils/Timers.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <android-base/strings.h>
#include <errno.h>
#include <android-base/file.h>
#include <mutex>

namespace android {

class MiuiForceDarkConfigImpl : public IMiuiForceDarkConfigStub, public virtual RefBase {
private:
    MiuiForceDarkConfigImpl() {}
    static MiuiForceDarkConfigImpl* sImplInstance;
    static std::mutex mInstLock;

public:
    virtual ~MiuiForceDarkConfigImpl() {}
    static MiuiForceDarkConfigImpl* getInstance();
    virtual void setConfig(float density, int mainRule, int secondaryRule, int tertiaryRule);
    virtual float getDensity();
    virtual int getMainRule();
    virtual int getSecondaryRule();
    virtual int getTertiaryRule();

protected:
    float mDensity;
    int mMainRule;
    int mSecondaryRule;
    int mTertiaryRule;
};

__attribute__((__weak__, visibility("default")))
extern "C" IMiuiForceDarkConfigStub* createForceDark();

__attribute__((__weak__, visibility("default")))
extern "C" void destroyForceDark(IMiuiForceDarkConfigStub* impl);

}
#endif // ANDROID_MIUI_FORTH_DARK_IMPL_H
