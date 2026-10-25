#ifndef _DISPLAY_EFFECT_PLATFORM_H_
#define _DISPLAY_EFFECT_PLATFORM_H_
#include "DisplayEffectBase.h"

namespace android {
class DisplayEffectPlatform : public DisplayEffectBase    
{
public:
    DisplayEffectPlatform(unsigned int displayId);
    ~DisplayEffectPlatform();
    int HandleEyeCare(int modeId, int cookie);
    int HandleCabcModeCustom(int modeId, int cookie);
    int HandleAD(int modeId, int cookie);
    int HandleHDR(int modeId, int cookie);
    int HandleDataBase(int value, int cookie);
    int HandleCameraRequest(int modeId, int cookie);
    int HandleGameMode(int modeId, int cookie);
    int Handle10Bit(int modeId, int cookie);
    void HandleDVStatus(int enable, int cookie);
    int onSuspend();
    int onResume();
    int mCABCstate = CABC_UI_ON;
};

} // namespace android
#endif
