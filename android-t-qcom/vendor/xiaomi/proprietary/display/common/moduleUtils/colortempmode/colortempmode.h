#ifndef _COLORTEMPMODE_H_
#define _COLORTEMPMODE_H_
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

namespace android {
class DisplayEffectBase;
class ColorTempMode: public RefBase
{
public:
    ColorTempMode(const sp<DisplayEffectBase>& display_effect);
    virtual ~ColorTempMode();
    int enableColorTempMode(int modeId, int cookie);
 private:
    sp<DisplayEffectBase> mDisplayEffect = nullptr;
}; //class ColorTempMode
} //namespace android

#endif