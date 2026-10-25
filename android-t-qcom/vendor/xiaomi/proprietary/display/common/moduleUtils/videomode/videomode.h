
#ifndef _VIDEOMODE_H_
#define _VIDEOMODE_H_
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <fstream>
#include <cutils/properties.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include "DisplayFeatureHal.h"
#include "parseXml.h"
#include "DisplayPostprocessing.h"
#include "display_effects.h"
#include "DSPP_Platform.h"

namespace android {
class DisplayEffectBase;
class VideoMode: public RefBase
{
public:
    VideoMode(const sp<DisplayEffectBase>& display_effect);
    virtual ~VideoMode();
    int enableVideoMode(int modeId, int cookie);
    void dump(std::string& result);
    void SetDisplayState(int display_state);
    int mVideoModeId = 0;
private:
    int GetExpertStatus();
    void RestoreAIDisplay();
    sp<DisplayEffectBase> mDisplayEffect = nullptr;
    int mDisplayState = kStateOn;
    sp<DSPP_Platform> mDSPP = nullptr;

}; // class VideoMode
} // namespace android

#endif

