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

#ifndef _DISPLAY_EFFECT_BASE_H_
#define _DISPLAY_EFFECT_BASE_H_

#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
//#include "display_utils.h"
#include "DisplayFeatureState.h"
#include "display_effects.h"
#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>
#include "cust_define.h"
#include "DisplayModuleUtils.h"
#include "FodSensorMonitor.h"
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include "truetone.h"
#include "videomode.h"
#include "paperMode.h"
#include "brightness.h"
#include "df_log.h"
#include "colortempmode.h"
#include "flatmode.h"
#include <mi_disp.h>
#ifndef MTK_PLATFORM
#include "displaycount.h"
#include <config/client_interface.h>
#include "sre.h"
#include "fpsmonitor.h"
#include "dither.h"
#include "sdr2hdr.h"

using snapdragoncolor::IMiStcService;

using namespace DisplayConfig;
#endif

typedef struct eyeCareParams {
    int nativeCTIndex;
    int eyeCareIndex[32];
    int restoreTime; //ms
} eyeCareParams;

typedef struct GABBParams {
    int luxTable[GABB_LUX_TABLE_LEN];
    int regTable[GABB_LUX_TABLE_LEN][GABB_PARAM_LEN];
    char cmd[5][64];
} GABBParams;

typedef struct DisplayAttributesInfo {
    uint32_t mode_index = 0;        /* mode index */
    uint32_t fps = 0;               /* Frame rate per second */
    uint32_t vsync_period_ns = 0;   /* VSync period in nanoseconds */
    uint32_t x_pixels = 0;          /* Total number of pixels in X-direction on the display panel */
    uint32_t y_pixels = 0;          /* Total number of pixels in Y-direction on the display panel */
} DisplayAttributesInfo;

namespace android {

enum HISTOGRAM_STATUS{
        HIST_STOPPED,
        HIST_STARTED,
};

enum HDR_EYPE {
    HDR10 = 0,
    CUVA_HDR = 1,
    DOLBY_VISION = 2,
};

class DisplayBCBC;
class DisplayFramerateSwitchThread;

class DisplayEffectBase : public AHandler
{
public:
    DisplayEffectBase(unsigned int displayId);
    virtual ~DisplayEffectBase();
    void start();

    virtual int HandleACNormal(int modeId, int cookie);
    virtual int HandleACCold(int cookie);
    virtual int HandleACWarm(int cookie);
    virtual int HandleIncreasedContrast(int cookie);
    virtual int HandleStandard(int cookie);
    virtual void SetCancelRestore(int value);
    virtual int SetEyeCareToStc(int modeId, int paperModeId);
    virtual int HandleEyeCare(int modeId, int cookie);
    virtual int HandleCabcModeCustom(int modeId, int cookie);
    virtual int HandleAD(int modeId, int cookie);
    virtual int HandleSunlightScreen(int modeId, int cookie);
    virtual int HandleNightMode(int modeId, int cookie);
    virtual int HandleHDR(int modeId, int cookie);
    virtual int Handle10Bit(int modeId, int cookie);
    virtual void HandleRegisterCallback(void * callback);
    virtual bool GetHdrStatus();
    virtual int getPanelName(const char *file_name,char *panel_name);
    virtual int parseXml();
    virtual int getEffectStatus();
    virtual int getEffectStatus(int index);
    virtual int featureEnablePolicy(int featureId, struct OutputParam *pSParams);
    virtual int setParams(int featureId, void* params);
    virtual void setBrightness(int value);
    virtual int handleCCBB(int brightness, int fMode);
    virtual int handleDispSwitchOption(int modeId, int cookie);
    virtual int setDisplayEffect(int fMode, struct OutputParam *pSParams);
    virtual void setDispParam(int displayId, int caseId, int modeId, int cookie);
    virtual void getDispParam(struct DispParam *pParam);
    virtual int GetMessageGeneration(int index);
    virtual void SetMessageGeneration(int index, int gen);
    virtual int CalculateGABBParms(int brightness);
    virtual int HandleGABB(int brightness);
    virtual int HandlePccGABB(int brightness);
    virtual int HandleEXTColorProc(int modeId, int cookie);
    static void *DisplayEventThread(void *);
    virtual int getDisplayState();
    virtual int HandleDisplayState(int modeId, int cookie);
    virtual int HandleDisplayMutualState(int displayId,int state);
    virtual int onResume();
    virtual int onSuspend();
    virtual void restoreQcomMode();
    virtual int setBreakCTDimming(int value);
    virtual int setBreakSLPCCDimming(int value);
    virtual int HandleCameraRequest(int modeId, int cookie);
    virtual int notifyQservice(int cmd, int value);
    virtual int HandleGameMode(int modeId, int cookie);
    virtual void setWCGState(bool enable);
    virtual void clearEyeCareStatus();
    virtual int HandleColorTemp(int modeId, int cookie);
    virtual int HandleDCBacklight(int value, int delay);
    virtual int handleDozeBrightnessMode(int modeId);

    virtual int HandleFpsSwitch(int modeId, int cookie);
    virtual int getDisplayAttributesInfo(void);
    virtual int notifySurfaceflingerFpsSwitch(int fps);
    virtual void setSettingsFps(int fps);
    virtual void setPowerLimitFps(int fps);
    virtual int HandleExpert(int caseId, int value);
    virtual void restoreExpertParam();
    virtual void saveExpertParam();
    virtual void ExpertParamValidCheck();
    virtual int restorePanelEffect();
    virtual void setAIDimParam(int step);
    virtual int setDCGamma(int enable);
    virtual void onCallback(int cmd, int value);

    /* add new*/
    virtual int HandlePaperMode(int modeId, int cookie);
    virtual int timerCallback(int caseId, int generation, int value);
    virtual int HandleDisplayScene(int modeId, int cookie);
    virtual void ApplySceneLut(int sceneId);
    virtual int HandleActiveApp(int modeId, int cookie);
    virtual int HandleDarkMode(int modeId, int cookie);
    virtual int getLimitFps();
    virtual void updateBacklightCoeff(float coeff);
    virtual void updateBrightness(int brightness);
    virtual void updateAodDitherStatus(int status);
    virtual int HandleTrueTone(int modeId, int cookie);
    virtual int HandleAdaptiveHDR(int modeId, int cookie);
    virtual void HandleLowBrightnessFOD(int state, int cookie);
    virtual int getGameModeId() { return mGameModeId;}
    virtual int HandleVideoMode(int modeId, int cookie);
    virtual int GetExpertMode();
    virtual void RestoreAIDisplay();
    virtual int GetVideoModeId();
    virtual int getFlatModeStatus();
    virtual void restoreBlPcc(int value);
    static void lockFpsRequest(int value);
    virtual void sensorCallback(int event, const Event &info);
    virtual void HandleTouchState(int caseId, int value);
    virtual int processDCBacklight(int brightness, int cookie);
    virtual char* printfMessage(int id);
    virtual void dumpInfo(std::string& result);
    virtual void HandleDVStatus(int enable, int cookie);
    virtual int HandleDataBase(int modeId, int cookie);
    virtual int HandleC3dLut(int modeId, int cookie);
    virtual int SetPicHDRToStc(int modeId, int cookie);
    virtual int HandlePicHDR(int modeId, int cookie);
/*Only M2 real function*/
    virtual int SetQsyncTimer(unsigned int displayId) { return 0; };
    virtual int CustomBrightnessProcess(int dbv);
    virtual void setDisplayEffectCustom(struct OutputParam *pSParams);

    enum {
        kLOW_BRIGHTNESS_FOD             = 'LFOD',
        kDISPLAY_SCENE                  = 'SCEN',
        kDISPLAY_ENV_ADAPTIVE_COLD      = 'Cold',
        kDISPLAY_ENV_ADAPTIVE_NORMAL    = 'Norm',
        kDISPLAY_ENV_ADAPTIVE_WARM      = 'Warm',
        kDISPLAY_COLOR_ENHANCE          = 'Ehan',
        kDISPLAY_STANDARD               = 'Stan',
        kDISPLAY_EYECARE                = 'EyeC',
        kDISPLAY_COLOR_TEMPERATURE      = 'CoTe',
        kDISPLAY_KEEP_WP_SRGB           = 'KeWP',
        kDISPLAY_CABC_MODE_SWITCH       = 'CABC',
        kDISPLAY_BRIGHTNESS_NOTIFY      = 'BrNo',
        kDISPLAY_NIGHT_MODE             = 'NigM',
        kDISPLAY_CCBB                   = 'CCBB',
        kDISPLAY_AD                     = 'AD  ',
        kDISPLAY_HDR                    = 'HDR ',
        kDISPLAY_HBM                    = 'HBM ',
        kDISPLAY_STATE_NOTIFY           = 'DsNo',
        kDISPLAY_EXT_COLOR_PROC         = 'EXCP',
        kDISPLAY_FOD_COLOR_MODE_P3      = 'FCP3',
        kDISPLAY_FOD_RESTORE_QCOM_MODE  = 'FRQM',
        kDISPLAY_FOD_SCREEN_ON          = 'FSCO',
        kDISPLAY_FOD_PRE_HBM            = 'FPRE',
        kDISPLAY_CAMERA                 = 'Cam ',
        kDISPLAY_SET_CALLBACK           = 'Clbk',
        kDISPLAY_BCBC_CUSTOM            = 'BCBC',
        kDISPLAY_GAME                   = 'Game',
        kDISPLAY_SUNLIGHT_SCREEN        = 'SLS ',
        kDISPLAY_DC_BACKLIGHT           = 'DCBL',
        kDISPLAY_DOZE_BRIGHTNESS_MODE   = 'DOZE',
        kDISPLAY_EXPERT                 = 'EXPT',
        kDISPLAY_VIDEO                  = 'ViDo',
        kDISPLAY_SENSOR_CONTROL         = 'SSCT',
        kDISPLAY_PAPER_MODE             = 'PaPr',
        kDISPLAY_TRUE_TONE              = 'TrTn',
        kDISPLAY_ACTIVE_APP             = 'ACTA',
        kDARK_MODE                      = 'Dark',
        kDISPLAY_STATE_NOTIFY_MUTUAL    = 'DNMu',
        kADAPTIVE_HDR                   = 'AHDR',
        kDISPLAY_DITHER_MODE            = 'DITH',
        kDISPLAY_TOUCH_STATE            = 'TPst',
        kDISPLAY_ORITATION_STATE        = 'ORIT',
        kDISPLAY_CUP_BLACK_COVERED_STATE = 'CUPb',
        kDISPLAY_10BIT                  = '10BT',
        kDISPLAY_SRE                    = 'SRE ',
        kDISPLAY_DOLBY_VISION_STATE     = 'DLBV',
        kDISPLAY_DATABASE               = 'DTBS',
        kDISPLAY_C3D_LUT                = 'C3DL',
        kDISPLAY_PIC_HDR                = 'PHDR',
    };

    unsigned int mDisplayId;
    sp<DisplayFramerateSwitchThread> mDisplayDFPS;
#ifdef HAS_QDCM
    sp<Sre> mDisplaySRE;
    sp<DisplayBCBC> mDisplayBCBC;
    sp<DisplayVSDR2HDR> mDisplayVSDR2HDR;
    sp<Dither> mDisplayIGCDither;
    sp<DisplayCountThread> mDisplayCount = nullptr;
#endif
    sp<VideoMode> mVideoMode = nullptr;
    sp<FlatMode> mFlatMode = nullptr;
    sp<ColorTempMode> mColorTempMode = nullptr;
    sp<MiBrightness> mBrightness;
    PccParams mPccParams;
    int curApp = NONE;
    int mPaperModeId = 0;
    int mLutId = -1;
    int mGameModeId;
    int mPanelDead = 0;
    bool mHistControl;
    bool mSRESupported = 0;
    int mRead = 0;
    int mInteracted_bl;
    int mHDRStatus = 0;
    int m10BitLayer = 0;
    struct DispParam mCurParam;
    bool mLocalHBMEnabled = false;
    sp<FodSensorMonitor> mFodSensorMonitor;
    int mVirtualNonZeroBl = 0;
    int mSoftBlMaxLevel;
    int mNativeBlMaxLevel;
    int mSoftBlThreshold;
    int mNativeBlThreshold;
    int mExpertEnable;
    /* 
     Type 0 for samsung panel, HBM cmd + dbv 0~2047
     Type 1 for CSOT panel, dbv 2048~4095
     */
    int mHbmType = 0;
    int mVirtualBl = 0;
    double mDCBLCoeff[2];
    std::vector<char> DCBLcmd0;
    std::vector<char> DCBLcmd1;
    std::vector<char> crcOffCmd;
    int mDCType = 0;
    int mFod110nitLux = 3;
    static void *ThreadWrapperPanel(void *);
    int threadFuncPanelMonitor();
    pthread_t mThreadPanelMonitor;
    int mWCGCorlorGamut = DATASPACE_SRGB;
    bool mWCGState = 0;
    int mDisplayState;
    int mSuspendFlag;
    int mMessageGeneration[CASE_MAX_STATE] = {0};
#ifdef HAS_QDCM
    ClientInterface *mDisplayConfigIntf = nullptr;
#endif
    int mDitherStrength = 5;
    int mDefaultGamut;
    int mDRELux;

protected:
    Mutex mEffectLock;
    Mutex mStatusLock;
    Mutex mHDRLock;
    Mutex mSetEffectLock;
    void setEffectStatus(int status);
    void _setEffectStatus(int status);
    int getEXTColorStatus();
    int getNightModeStatus();
    int getEyecareParams(unsigned int level);
    int getEffectFirstId(int status);
    int getSceneId();

    virtual void onMessageReceived(const sp<AMessage> &msg);
    GlobalPAParam mPaParam;
    sp<TrueTone> mTrueTone = nullptr;
private:
    void notifySFWcgState(bool enable);
    bool checkModeForWCG(int caseId, int modeId);
    bool checkWCGEnable(int caseId, int modeId, int cookie);
    int registerPollEvent();
    int DisableLut();
    void dumpStc(std::string& result);
    bool InputParamValidCheck(int caseId, int value);
    sp<PaperMode> mPaperMode = nullptr;
    sp<ALooper> mLooper;
    sp<DisplayFeatureState> mStates[SET_MAX_FEATURE_CNT];
    int mEffectStatus;
    bool  mDetectStatus;
    int mCurrentBL;
    Mutex mBLLock;
    int mHistBrightnessThreshold;
    bool mCTDimming;
    int mHasEXTColorProc;

    int mHDRSwitch = 1;
    int mCancelRestore;
    int mHbmForceCloseGeneration = -1;

    int mTypBrightness;
    int mStartBrightness;
    float mBrightnessStep;
    int mReadMaxBrightnessEnable;

    bool mBacklightPoll;
    pthread_t mThreadBacklight;
    pollfd mPollBlFd[2];

    eyeCareParams mEyeCareParams;
    GABBParams mGABBParams;
    const int mEyeCareValues[32] = {42, 38, 36, 34, 33, 29, 28, 27,
                            23, 21, 18, 16, 13, 11,
                            8, 5, 3, 0, -2, -5,
                            -7, -10, -12, -14, -17, -19,
                            -22, -27, -29, -32, -36, -38};
    int mBlLut[400] = {50,50,50,50,50,50,50,50,50,50,50,50,50,51,51,51,51,51,51,51,51,51,52,52,52,52,52,52,53,53,53,53,53,
                      54,54,54,54,54,55,55,55,55,56,56,56,57,57,57,57,57,58,58,58,58,59,59,59,59,59,60,60,60,60,61,61,61,
                      61,62,62,62,63,63,63,64,64,65,65,65,66,66,66,67,67,68,68,68,69,69,70,70,70,71,71,71,72,72,73,73,74,
                      74,74,75,75,76,76,76,77,77,78,78,79,79,79,80,80,81,81,82,82,83,83,83,84,84,85,85,86,86,87,87,87,88,
                      88,89,89,90,90,91,91,91,92,92,93,93,94,94,95,95,96,96,96,97,97,98,98,99,99,100,100,101,101,102,102,
                      102,103,103,104,104,105,105,106,106,107,107,107,108,108,109,109,110,110,110,110,111,111,111,111,112,
                      112,112,112,113,113,113,113,114,114,114,114,115,115,115,115,116,116,116,116,117,117,117,117,118,118,
                      118,118,119,119,119,119,120,120,120,120,121,121,121,121,122,122,122,123,123,123,124,124,124,125,125,
                      125,126,126,126,127,127,127,128,128,128,129,129,129,130,130,130,131,131,131,132,132,132,133,133,133,
                      134,134,134,135,135,135,136,136,136,137,137,137,138,138,138,139,139,139,140,140,141,141,142,142,143,
                      143,144,144,145,145,146,146,147,147,148,148,149,149,150,150,151,151,152,152,153,153,154,154,155,155,
                      156,156,157,157,158,158,159,159,160,160,161,161,162,162,163,163,164,164,165,165,166,166,167,167,168,
                      168,169,169,170,170,171,171,172,172,173,173,174,174,175,175,176,176,177,177,178,178,179,179,180,180,
                      181,181,182,182,183,183,184,184,185,185,186,186,187,187,188,188,189,189,190,190,191,191,192,192,193,
                      193,194,194,195,195,196,196,197,198,198,199,199};
    int mBlLutDC[400] = {50,50,50,50,50,50,50,50,50,50,50,50,50,51,51,51,51,51,51,51,51,51,52,52,52,52,52,52,53,53,53,53,53,
                      54,54,54,54,54,55,55,55,55,56,56,56,57,57,57,57,57,58,58,58,58,59,59,59,59,59,60,60,60,60,61,61,61,61,
                      62,62,62,63,63,63,64,64,65,65,65,66,66,66,67,67,68,68,68,69,69,70,70,70,71,71,71,72,72,73,73,74,74,74,
                      75,75,76,76,76,77,77,78,78,79,79,79,80,80,81,81,82,82,83,83,83,84,84,85,85,86,86,87,87,87,88,88,89,89,
                      90,90,91,91,91,92,92,93,93,94,94,95,95,96,96,96,97,97,98,98,99,99,100,100,101,101,102,102,102,103,103,
                      104,104,105,105,106,106,107,107,107,108,108,109,109,110,110,110,110,111,111,111,111,112,112,112,112,113,
                      113,113,113,114,114,114,114,115,115,115,115,116,116,116,116,117,117,117,117,118,118,118,118,119,119,119,
                      119,120,120,120,120,121,121,121,121,122,122,122,123,123,123,124,124,124,125,125,125,126,126,126,127,127,
                      127,128,128,128,129,129,129,130,130,130,131,131,131,132,132,132,133,133,133,134,134,134,135,135,135,136,
                      136,136,137,137,137,138,138,138,139,139,139,140,140,141,141,142,142,143,143,144,144,145,145,146,146,147,
                      147,148,148,149,149,150,150,151,151,152,152,153,153,154,154,155,155,156,156,157,157,158,158,159,159,160,
                      160,161,161,162,162,163,163,164,164,165,165,166,166,167,167,168,168,169,169,170,170,171,171,172,172,173,
                      173,174,174,175,175,176,176,177,177,178,178,179,179,180,180,181,181,182,182,183,183,184,184,185,185,186,
                      186,187,187,188,188,189,189,190,190,191,191,192,192,193,193,194,194,195,195,196,196,197,198,198,199,199};

    long maxBLLevel;
    float mLastLux = 0.0;
    bool mDisableDC = false;
    bool dc_status_modified = false;
    bool mDCEnabled = false;
    int mSysNonZeroBl = 0;
    bool mFlatOn = false;
    int mDozeBrightnessMode = 0;

    bool first_cycle = true;
    bool mFoldSensorStarted = false;
    std::vector<DisplayAttributesInfo> display_attributes_info = {};
    int current_mode_index = -1;
    int current_fps = 0;
    int settings_fps = 0;
    int last_app_fps = 0;
    int power_limit_fps = 0;
    int thermal_limit_fps = 0;
    const string mPollFdPath[POLL_FD_NUM] = {"/sys/class/backlight/panel0-backlight/brightness_clone",
                                             "/sys/class/drm/card0-DSI-1/fod_ui_ready"};
    static constexpr const char* kDispFeaturePath = "/dev/mi_display/disp_feature";
    pollfd mDFPollFd;
    unsigned int mTimerGeneration[TIMER_STATE_MAX] = {0};
    int mSceneId = -1;
    int mDarkMode = 0;
    int mDitherEn = 1;
    int mFFCheckNum = 49;
    std::vector<int> mFFData;
    int mAdaptiveHDR = 0;
};

};
#endif
