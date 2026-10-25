/*
 * Copyright (C) 2013 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _DISPLAY_EFFECT_H_
#define _DISPLAY_EFFECT_H_

#include <utils/Singleton.h>
#include <utils/StrongPointer.h>
#include <string>
#include <stdint.h>
#include <map>

#include "IDisplayFeatureControl.h"


#define SET_AD IDisplayFeatureControl::SET_AD
#define SET_PANEL_MODE IDisplayFeatureControl::SET_PANEL_MODE
#define SET_QUALCOMM_MODE IDisplayFeatureControl::SET_QUALCOMM_MODE
#define SET_PCC_MODE IDisplayFeatureControl::SET_PCC_MODE
#define SET_COLOR_TEMPERATURE IDisplayFeatureControl::SET_COLOR_TEMPERATURE
#define SET_SVI IDisplayFeatureControl::SET_SVI
#define SET_LTM IDisplayFeatureControl::SET_LTM

using std::string;
using std::map;
using std::make_pair;
using std::pair;

namespace android {

class DisplayEffect: public Singleton<DisplayEffect>
{
public:
    DisplayEffect();
    ~DisplayEffect();

    int HandleACNormal(int cookie, int brightness);
    int HandleACCold(int cookie, int brightness);
    int HandleACWarm(int cookie, int brightness);
    int HandleIncreasedContrast(int cookie);
    int HandleStandard(int cookie);
    int HandleEyeCare(int modeId, int cookie);
    int HandleCabcModeCustom(int modeId, int cookie);
    int HandleKeepWhitePointSRGB(int modeId, int cookie);
    int HandleBrightnessInteract(int modeId, int cookie);
    int HandleCTAdapt(int modeId, int cookie);
    int HandleNightMode(int modeId, int cookie);
    int HandleHBM(int modeId, int cookie);
    int HandleHDR(int modeId, int cookie);
    int HandleAD(int modeId, int cookie);
    int HandleSunlightScreen(int modeId, int cookie);
    void NotifyBrightness(int brightness);
    int featureEnablePolicy(int featureId, int value,
        int cookie, int caseid = 0, bool enable = true);
    int setDisplayEffect(int fMode, int value, int cookie);
    void sendDisplayEvent(int event);
    void sendDisplayEventExt(int event, int value, int cookie);
    int handleDisplayRequest(int displayId, int caseId, int modeId, int cookie);
    int handleLuminousScreen(int brightness, int fMode);
    int handleCCBB(int brightness, int fMode);
    int handleDispSwitchOption(int modeId, int cookie);
    void setInt32(string name, int value);
    int getInt32(string name, int *value);
    int HandleCameraRequest(int modeId, int cookie);
    int HandleGameMode(int modeId, int cookie);
    int HandleDCBacklight(int modeId, int cookie);
    int HandleFpsSwitch(int modeId, int cookie);
    int handleDozeBrightnessMode(int modeId, int cookie);
    int HandleExpert(int caseId, int value);
    int HandleTouchState(int caseId, int value);
    int HandleOritationState(int caseId, int value);
    int HandleCupBlackCoveredState(int caseId, int value);
    int HandleVideoMode(int modeId, int cookie);
    int HandleActiveApp(int modeId, int cookie);
    int HandleSmartDfpsState(int modeId, int cookie);
    int HandleFrameDump(int modeId, int cookie);
    int HandlePaperMode(int modeId, int cookie);
    int HandleTrueTone(int modeId, int cookie);
    int HandleColorTempMode(int modeId, int cookie);
    int HandleBCBC(int modeId, int cookie);
    int HandleDisplayScene(int modeId, int cookie);
    int HandleDarkMode(int modeId, int cookie);
    int HandleAdaptiveHDR(int modeId, int cookie);
    int HandleDither(int modeId, int cookie);
    int HandleDVStatus(int modeId, int cookie);
    int HandlePicHDR(int modeId, int cookie);
    void set_displayId(int displayId);
    int get_displayId();

protected:
    Mutex mEffectLock;
    Mutex mStatusLock;
    int getEffectStatus();
    int getEffectStatus(int index);
    void setEffectStatus(int status);
    int getNightModeStatus();
    int getEyecareParams(unsigned int level);
    int enableEyecareNightMode(int brightness, int fMode);

private:
    friend class Singleton<DisplayEffect>;
    int mEffectStatus;
    Mutex mDataLock;
    map<string,int> mData;

    int m_displayId;
};

};
#endif
