/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

#ifndef ANDROID_MI_SURFACEFLINGER_IMPL_H
#define ANDROID_MI_SURFACEFLINGER_IMPL_H

#include <android-base/thread_annotations.h>
#include <compositionengine/OutputLayer.h>
#include "IMiSurfaceFlingerStub.h"
#include "TouchMonitorThread.h"
#include "FodMonitorThread.h"
#include "BacklightMonitorThread.h"
#include "PenMonitorThread.h"
#include "FingerPrintDaemonGet.h"
#include "DisplayFeatureGet.h"
#include "Layer.h"
#include "BufferQueueLayer.h"
#include "RefreshRateOverlay.h"
#include "BufferStateLayer.h"
#include "SurfaceFlinger.h"
#include "Scheduler/Scheduler.h"
#include <compositionengine/impl/Output.h>
#include <compositionengine/LayerFE.h>
#include <gui/WindowInfo.h>
#include <gui/IMiSurfaceComposerStub.h>

#ifdef QTI_DISPLAY_CONFIG_ENABLED
#include "QsyncFeature.h"
#endif

namespace android {

class FingerPrintDaemonGet;
class FingerprintStateManager;
class FPMessageThread;
class TouchMonitorThread;
class FodMonitorThread;
class BacklightMonitorThread;
class PenMonitorThread;
class DisplayFeatureGet;
class Layer;
class SurfaceFlinger;
class DisplayDevice;

#ifdef QTI_DISPLAY_CONFIG_ENABLED
class QsyncFeature;
#endif

#define  MI_FOD_UNLOCK_SUCCESS  0x10000000

enum LayerTypes {
  LAYER_UNKNOWN = 0,
  LAYER_APP = 1,
  LAYER_GAME = 2,
  LAYER_BROWSER = 3,
};

using CompositionRefreshArgs= compositionengine::CompositionRefreshArgs;
class MiSurfaceFlingerImpl : public IMiSurfaceFlingerStub {
public:
    enum { // idle timeout  interval
        //  tp idle timeout  interval 2500ms + tp delay 500ms
        eLtpoTpIdleTimeoutInterval = 2500,
        //  idle timeout  interval 1100ms + tp delay 500ms
        eIdleTimeoutInterval = 1100,
        //  tp idle timeout  interval 3000ms
        eTpIdleTimeoutInterval = 3000,
        //  tp idle timeout  interval 1000ms
        eTpTimeoutInterval = 1000,
        //  tp idle timeout  short interval 10ms
        eTpTimeoutShortInterval = 10,
        //  tp idle timeout  interval 1500ms
        ePromationTpIdleTimeoutInterval = 1500,
        //  fps stat timeout  interval 6000ms
        eFpsStatTimeoutInterval = 6000,
    };

    enum FPS_SWITCH_MODULE {
        FPS_SWITCH_VIDC       = 243,
        FPS_SWITCH_DEFAULT    = 244,
        FPS_SWITCH_WHITE_LIST = 245,
        FPS_SWITCH_LAUNCHER   = 246,
        FPS_SWITCH_BENCHMARK  = 247,
        FPS_SWITCH_VIDEO      = 248,
        FPS_SWITCH_TOPAPP     = 249,
        FPS_SWITCH_POWER      = 250,
        FPS_SWITCH_MULTITASK  = 251,
        FPS_SWITCH_CAMERA     = 252,
        FPS_SWITCH_THERMAL    = 253,
        FPS_SWITCH_GAME       = 254,
        FPS_SWITCH_SETTINGS   = 255,
        FPS_SWITCH_SMART      = 256,
        FPS_SWITCH_EBOOK   = 257,
        FPS_SWITCH_TP_IDLE      = 258,
        FPS_SWITCH_SHORT_VIDEO     = 259,
        FPS_SWITCH_PROMOTION = 260,
        FPS_SWITCH_HIGH_FPS_VIDEO = 261,
        FPS_SWITCH_POWER_FULL_ON_CHARGER = 263,
        FPS_SWITCH_ULTIMATE_PERF = 264,
        FPS_SWITCH_MAP_QSYNC = 265,
        FPS_SWITCH_VIDEO_FULL_SCREEN = 266,
        FPS_SWITCH_AUTOMODE = 267,
        FPS_SWITCH_ANIMATION_SCREEN = 268,
        FPS_SWITCH_GOOGLE_VRR = 269,
        FPS_SWITCH_INPUT = 270,
        FPS_SWITCH_LOWER_VIDEO = 271,
        FPS_SWITCH_MAX = FPS_SWITCH_LOWER_VIDEO,
    };

    enum {
        MI_VIDEO_FPS_22HZ = 23,
        MI_VIDEO_FPS_24HZ = 24,
        MI_VIDEO_FPS_25HZ = 25,
        MI_VIDEO_FPS_27HZ = 27,
        MI_VIDEO_FPS_30HZ = 30,
        MI_VIDEO_FPS_33HZ = 33,
        MI_VIDEO_FPS_40HZ = 40,
        MI_VIDEO_FPS_43HZ = 43,
        MI_VIDEO_FPS_60HZ = 60,
        MI_VIDEO_FPS_63HZ = 63,
        MI_VIDEO_FPS_66HZ = 66,
        MI_VIDEO_FPS_90HZ = 90,
        MI_VIDEO_FPS_93HZ = 93,
        MI_VIDEO_FPS_96HZ = 96,
        MI_VIDEO_FPS_120HZ = 120,
    };

    enum {
        MI_SETTING_FPS_30HZ = 30,
        MI_SETTING_FPS_60HZ = 60,
        MI_SETTING_FPS_90HZ = 90,
        MI_SETTING_FPS_120HZ = 120,
    };

    enum { // flags for doTransaction()
        // should diffrent form enums defined in layer.h
        // like:eInputInfoChanged in layer.h
        // EXTRA_FLAG_IS_SCREEN_PROJECTION in MiuiWindowManager.java
        eScreenProjection = 0x01000000,
        // EXTRA_FLAG_IS_CALL_SCREEN_PROJECTION in MiuiWindowManager.java
        eInCallScreenProjection = 0x00004000,
        //EXTRA_FLAG_IS_SCREEN_SHARE_PROTECTION_WITH_BLUR in MiuiWindowManager.java
        eVirtualdisplayCoverBlur = 0x00000001,
        //EXTRA_FLAG_IS_SCREEN_SHARE_PROTECTION_HIDE in MiuiWindowManager.java
        eVirtualdisplayHide = 0x00000002,
    };

    enum {
        CPUCOMBTYPE_4_4       = 1,
        CPUCOMBTYPE_4_3_1     = 2,
        CPUCOMBTYPE_6_2       = 3,
        CPUCOMBTYPE_NOT_EXIST = 10,
    };

    MiSurfaceFlingerImpl();
    int init(sp<SurfaceFlinger> flinger) override;
    void init_property();
    void init_property_old();
    void queryPrimaryDisplayLayersInfo(const std::shared_ptr<android::compositionengine::Output> output,
                               uint32_t& layersInfo) override;
    bool hookMiSetColorTransform(const compositionengine::CompositionRefreshArgs& args,
                                 const std::optional<android::HalDisplayId> halDisplayId,
                                 bool isDisconnected);
#ifdef MI_FEATURE_ENABLE
    void setAlphaInDozeState();
#endif
    bool checkVideoFullScreen();
    void checkLayerStack() override;
    void prePowerModeFOD(hal::PowerMode mode) override;
    void postPowerModeFOD(hal::PowerMode mode) override;
    void notifySaveSurface(String8 name) override;
    void lowBrightnessFOD(int32_t state) override;
    void writeFPStatusToKernel(int fingerprintStatus, int lowBrightnessFOD) override;
    void updateMiFodFlag(compositionengine::Output *output,
                         const compositionengine::CompositionRefreshArgs& refreshArgs) override;
    void updateFodFlag(int32_t flag) override;
    void updateUnsetFps(int32_t flag) override;
    void updateLHBMFodFlag(int32_t flag) override;
    int32_t getCurrentSettingFps();
    void changeFps(int32_t fps, bool fixed, bool isUseQsyncGroup = false);
    status_t enableSmartDfps(int32_t fps) override;
    void updateSystemUIFlag(int32_t flag) override;
    void saveFodSystemBacklight(int backlight) override;
    bool validateLayerSize(uint32_t w, uint32_t h) override;
    void getHBMLayerAlpha(Layer* layer, float& alpha) override;

    double getHBMOverlayAlphaLocked();
    void updateHBMOverlayAlphaLocked(int32_t brightness) REQUIRES(mFlinger->mStateLock);
    void updateIconOverlayAlpha(half alpha);
    void forceSkipTouchTimer(bool skip);
    void forceSkipIdleTimer(bool skip);
    void setDisplayFeatureGameMode(int32_t game_mode);
    void initSecondaryDisplayLocked(Vector<DisplayState>& displays) ;
    sp<DisplayDevice> getSecondaryDisplayDeviceLocked() REQUIRES(mFlinger->mStateLock);

    float updateCupMuraEraseLayerAlpha(unsigned long brightness);

    void setGameArgs(CompositionRefreshArgs &refreshArgs) override;
    void saveDisplayColorSetting(DisplayColorSetting displayColorSetting) override;
    void updateGameColorSetting(String8 gamePackageName, int gameColorMode, DisplayColorSetting& mDisplayColorSetting) override;

    void updateForceSkipIdleTimer(int mode,bool isPrimary) override;

    void launcherUpdate(Layer* layer) override;
    bool allowAospVrr() override;
    bool isModeChangeOpt() override;
    bool isLtpoPanel() override;
    bool allowCtsPass(std::string name) override;
    bool isResetIdleTimer() override;
    bool isVideoScene() override;
    int getCurrentPolicyNoLocked();
    bool isSupportLowBrightnessFpsSwitch(int32_t current_setting_fps);
    bool isForceSkipIdleTimer() override;
    bool isMinIdleFps(nsecs_t period) override;
    bool isTpIdleScene(int32_t current_setting_fps) override;
    bool isSceneExist(scene_module_t scene_module) override;
    nsecs_t setMinIdleFpsPeriod(nsecs_t period) override;
    bool isDdicIdleModeGroup(int32_t modeGroup) override;
    bool isDdicAutoModeGroup(int32_t modeGroup) override;
    bool isDdicQsyncModeGroup(int32_t modeGroup) override;
    bool isNormalModeGroup(int32_t modeGroup) override;
    uint32_t getDdicMinFpsByGroup(int32_t modeGroup) override;
    int32_t getGroupByBrightness(int32_t fps, int32_t modeGroup) override;
    DisplayModePtr setIdleFps(int32_t mode_group, bool *minFps) override;
    DisplayModePtr setVideoFps(int32_t mode_group) override;
    DisplayModePtr setTpIdleFps(int32_t mode_group) override;
    DisplayModePtr setPromotionModeId(int32_t defaultMode) override;
    void isRestoreFps(bool is_restorefps, int32_t lock_fps);
    void setLockFps(DisplayModePtr mode);
    status_t updateDisplayLockFps(int32_t lockFps, bool ddicAutoMode = false, int ddicMinFps = 0) override;
    status_t updateGameFlag(int32_t flag) override;
    status_t updateVideoInfo(s_VideoInfo *videoInfo) override;
    const char* getSceneModuleName(const scene_module_t& module) const;
    status_t updateVideoDecoderInfo(s_VideoDecoderInfo *videoDecoderInfo) override;

    Mutex mVideoDecoderMutex;
    std::vector<s_VideoDecoderInfo> mVideoDecoderFps;
    float mVideoDecoderFrameRate;
    float mVideoDecoderFrameRateReal;

    void updateFpsRangeByLTPO(DisplayDevice* displayDevice, FpsRange* fpsRange) override;
    void setDVStatus(int32_t enable) override;
    void setVideoCodecFpsSwitchState(void);
    const char* getFpsSwitchModuleName(const int& module) const;
    void updateScene(const int& module, const int& value, const String8& pkg) override;
    void setLayerRealUpdateFlag(uint32_t scense, bool flag) override;

    std::vector<int> primaryIdleRefreshRateList;
    std::vector<int> primaryIdleBrightnessList;
    std::vector<int> secondaryIdleRefreshRateList;
    std::vector<int> secondaryIdleBrightnessList;
    void parseIdleFpsByBrightness(disp_display_type displaytype, const std::string& params);
    void splitString(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters = " ");

    std::vector<int> mFpsStatValue;
    unsigned long mFpsStatTimerCount;
    std::optional<scheduler::OneShotTimer> mFpsStatTimer;
    void FpsStatTimerClean();
    void FpsStatTimerCallback(scheduler::Scheduler::TimerState state);
    void resetFpsStatTimer();
    scheduler::Scheduler::TimerState fpsTimer = scheduler::Scheduler::TimerState::Reset;
    void signalLayerUpdate(uint32_t scense, sp<Layer> layer) override;
    void notifyDfAodFlag(int32_t flag);
    void updateAodRealFps(bool hasAOD);
    void notifyDfLauncherFlag(int32_t flag);
    bool isAutoModeAllowed(void);
    void saveDisplayDozeState(unsigned long doze_state);
    void addDolbyVisionHdr(std::vector<ui::Hdr> &hdrTypes);
    void disableOutSupport(bool* outSupport) override;
    unsigned long getCurrentBrightness();
    void saveDefaultResolution(DisplayModes supportedModes) override;
    void setAffinity(const int& affinityMode) override;
    void setreAffinity(const int& reTid) override;
    // BSP-Game: add Game Fps Stat
    sp<Layer> getGameLayer(const char* activityName) override;
    bool isFpsStatSupported(Layer* layer) override;
    void createFpsStat(Layer* layer) override;
    void destroyFpsStat(Layer* layer) override;
    void insertFrameTime(Layer* layer, nsecs_t time) override;
    void insertFrameFenceTime(Layer* layer, const std::shared_ptr<FenceTime>& fence) override;
    bool isFpsStatEnabled(Layer* layer) override;
    void enableFpsStat(Layer* layer, bool enable) override;
    void setBadFpsStandards(Layer* layer, int targetFps, int thresh1, int thresh2) override;
    void reportFpsStat(Layer* layer, Parcel* reply) override;
    bool isFpsMonitorRunning(Layer* layer) override;
    void startFpsMonitor(Layer* layer, int timeout) override;
    void stopFpsMonitor(Layer* layer) override;
    // END

    void setFrameBufferSizeforResolutionChange(sp<DisplayDevice> displayDevice) override;

    Mutex mUpdateSceneMutex;
    unsigned long mSkipIdleThreshold;
    // The last promotion refresh rate
    DisplayModeId mLastPromotionModeId;
   // Flag used to support promotion fps for 90hz
    bool mIsSkipPromotionFpsfor_90hz = false;
   // Flag used to support auto mode for max fps setting
    bool mIsSupportAutomodeForMaxFpsSetting = false;
   // Flag used to support auto mode for tp idle
    bool mIsSupportAutomodeTpIdle = false;
    // Flag used to support tp idle for low brightness
    bool mIsSupportTpIdleLowBrightness = true;
    // Flag used to skip idle
    bool mIsSupportSkipIdle = false;
    // Flag used to skip idle for 90hz
    bool mIsSupportSkipIdlefor_90hz = false;
    // Flag used to sync tp
    bool mIsSyncTpForLtpo = false;
    // Flag used to force use hwc backlight.
    bool mForceHWCBrightness = false;
    // Flag used to support touch idle
    bool mIsSupportTouchIdle = false;
    // Flag used to set idle flag for smartpen
    bool mIsSupportSmartpenIdle = false;
    // Flag used to support ultimate perf
    bool mIsSupportUltimatePerf = false;
    // The ltpo panel support
    bool mIsLtpoPanel;
    // Flag used to set idle flag
    bool mSetIdleForceFlag = false;
    // Flag used to set idle flag for command
    bool mSetCmdLockFpsFlag = false;
    // Flag used to set idle flag for pcc/brightness
    bool mSetLockFpsFlag = false;
    // Flag used to set idle flag for power full on charger
    bool mSetLockFpsPowerFullFlag = false;
    // Flag used to set full screen for video
    bool mSetVideoFullScreenFlag = false;
    // Flag used to set google vrr flag
    bool mSetGoogleVrrFlag = false;
    // Flag used to set automode flag
    bool mSetAutoModeFlag = true;
    // Flag used to set lower video flag
    bool mSetLowerVideoFlag = false;
    // Flag used to set input flag
    bool mSetInputFlag = false;
    // Flag used to set animation flag
    bool mSetAnimationFlag = false;
    // Flag used to set current qsync flag
    bool mSetQsyncFlag = false;
    // Flag used to set qsync vsync flag
    int mSetQsyncVsyncRate = 0;
    // Flag used to support force skip idle when the power is full with the charger
    bool mIsSupportLockFpsByPowerFull = false;
    // The idle refresh rate
    int mIdleFrameRate;
    // The fod Monitor rate
    float mFodMonitorFrameRate;
    // Flag used to set promotion flag
    bool mSetPromotionFlag = false;
    // The video and camera refresh rate support
    bool mIsSupportVideoOrCameraFrameRate;
    // The video refresh rate
    float mVideoFrameRate = MI_VIDEO_FPS_60HZ;
    // The video last refresh rate
    float mVideoLastFrameRate = MI_VIDEO_FPS_60HZ;
    // The video refresh rate from app
    float mAppVideoFrameRate = false;
    // Flag used to set video flag
    bool mSetVideoFlag = false;
    // Flag used to set camera flag
    bool mSetCameraFlag = false;
    // Flag used to set shot video flag
    bool mSetShotVideoFlag = false;
    // Flag used to set video high fps flag
    bool mSetVideoHighFpsFlag = false;
    // power mode status
    hal::PowerMode mPowerMode;
    // Flag used to set game flag
    bool mSetGameFlag = false;
    // Flag used to set bullet chat state
    bool mSetBulletChatState = false;
    // Flag used to set tp idle flag, false for tp idle, true for aosp idle
    bool mSetTpIdleFlag = true;
    // Flag used to set ltpo tp flag
    bool mSetLtpoTpIdleFlag = false;
    // Flag used to set Ultimate performance flag, when true, Force disable smartfps
    bool mSetUltimatePerfFlag = false;
    // Flag used to set ebook idle flag
    bool mSetEbookIdleFlag = false;
    // Flag used to set layer1 update flag,  true for updating, but status bar update is false
    bool mLayerRealUpdateFlag1;
    // Flag used to set layer2 update flag,  true for updating, but status bar update is false
    bool mLayerRealUpdateFlag2;
    // Flag used to set aod flag
    bool mSetAodFlag = false;
    bool mSetStatusFlag = false;
    bool mSetVsyncOptimizeFlag = false;
    int mTouchflag = 0;
    // The tolerance within which we consider FPS approximately equals.
    static constexpr float FPS_EPSILON = 0.001f;
    //!! if you want change  mAllowAospVrr,please change init
    bool mAllowAospVrr = true;
    // Flag used to set refreshrate mode change optimization flag
    bool mIsModeChangeOpt = false;
    bool mIsCts = false;
    bool fpsStrategy = true;

    bool mCupMuraEraselayerEnabled = false;

    bool mPreHasCameraCoveredLayer = false;
    bool mCCTNeedCheckTouch = false;

    int32_t mCurrentSettingFps = 0;
    int32_t mUnsetSettingFps = 0;
    int isFod();
#ifdef MI_FEATURE_ENABLE
    // enable of protect of doze state
    bool mProtectDozeState = false;
    // enable of protect of doze state
    bool mMarkAlpha = false;
    // last power mode status only for aod
    hal::PowerMode mLastPowerModeAOD = hal::PowerMode::OFF;
#endif

    void HandleDisplayFeatureCallback(int event, int value, float r, float g, float b);
    bool isCallingBySystemui(const int callingPid) override;
    void setOrientationStatustoDF() override;
    bool isFold();
    bool isDualBuitinDisp();
    bool isCTS();
    bool isFodEngineEnabled();
    status_t updateRefreshRate(int force, int mode);
    sp<DisplayDevice> getActiveDisplayDevice();
    status_t setDesiredMultiDisplayPolicy(const sp<DisplayDevice>& display,
            const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy);
    status_t setDesiredMultiDisplayModeSpecsInternal(const sp<DisplayDevice>& display,
        const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy);
    void setupIdleTimeoutHandle(uint32_t hwcDisplayId, bool enableDynamicIdle, bool isPrimary);
    void dumpDisplayFeature(std::string& result);
    bool isSecondRefreshRateOverlayEnabled();
    bool isEmbeddedEnable();
    void changeLayerDataSpace(Layer* layer);

    // Schedule an asynchronous or synchronous task on the main thread.
    template <typename F, typename T = std::invoke_result_t<F>>
    [[nodiscard]] std::future<T> schedule(F&&);

    sp<DisplayFeatureGet> mDisplayFeatureGet;

#if MI_SCREEN_PROJECTION
    void setDiffScreenProjection(uint32_t isScreenProjection, Output* outPut);

    void setLastFrame(uint32_t isLastFrame, Output* outPut);

    void setCastMode(uint32_t isCastMode, Output* outPut);

    bool isNeedSkipPresent(Output* outPut);

    void rebuildLayerStacks(Output* outPut);

    void ensureOutputLayerIfVisible(Output* outPut, const compositionengine::LayerFECompositionState* layerFEState);

    bool includesLayer(const sp<compositionengine::LayerFE>& layerFE, const Output* outPut);

    bool continueComputeBounds(Layer* layer);
    void prepareBasicGeometryCompositionState(
                  compositionengine::LayerFECompositionState* compositionState, const Layer::State& drawingState, Layer* layer);
    bool setScreenFlags(int32_t screenFlags, Layer* layer);
    void setChildImeFlags(uint32_t flags, uint32_t mask, Layer* layer);
    String8 getLayerProtoName(Layer* layer, const Layer::State& state);
    void fillInputInfo(gui::WindowInfo* info, Layer* layer);
    uint32_t isCastMode();
    void setIsCastMode(uint32_t castMode);
    uint32_t isLastFrame();
    void setIsLastFrame(uint32_t lastFrame);
    void setupNewDisplayDeviceInternal(sp<DisplayDevice>& display, const DisplayDeviceState& state);
    void setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays, uint32_t flags,
                                                 int64_t desiredPresentTime);
    void applyMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays, uint32_t* flags) REQUIRES(mFlinger->mStateLock);
    uint32_t setMiuiDisplayStateLocked(const MiuiDisplayState& s) REQUIRES(mFlinger->mStateLock);
    void setClientStateLocked(uint64_t what,sp<Layer>& layer,uint32_t* flags, const layer_state_t& s);
    String8 getDumpsysInfo();
    bool showContineTraverseLayersInLayerStack(const sp<Layer>& layer);
    inline bool hideRecorderBar(const DisplayDeviceState& state) {
        return state.isVirtual() && state.displayName.find("StableScreenRecorderCore") != std::string::npos;
    }
#endif

#if MI_VIRTUAL_DISPLAY_FRAMERATE
    bool skipComposite(const sp<DisplayDevice>& display);
    void setLimitedFrameRate(const sp<DisplayDevice>& display, const DisplayDeviceState& state);
    void updateLimitedFrameRate(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState);
    void updateDisplayStateLimitedFrameRate(uint32_t what, DisplayDeviceState& state,
            const DisplayState& s, uint32_t &flags);
    void initLimitedFrameRate(DisplayState& d);
    void setLimitedFrameRateDisplayDevice(DisplayDevice * display, uint32_t frameRate);
    bool needToCompositeDisplayDevice(DisplayDevice * display);
#endif
    // MIUI+ send SecureWin state
    void setSecureProxy(sp<IBinder> secureProxy);
    sp<IBinder> getSecureProxy();
    void updateOutputExtra(std::shared_ptr<compositionengine::Output> output, bool isInternalDisplay, int sequenceId) override;
    void handleSecureProxy(compositionengine::Output*, uint32_t layerStackId) override;
    // END

#ifdef MI_SF_FEATURE
    void setNewMiSecurityDisplayState(sp<DisplayDevice>& display, const DisplayDeviceState& state);
    void setMiSecurityDisplayState(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState);
    bool getMiSecurityDisplayState(const compositionengine::Output* output);
    void setVirtualDisplayCoverBlur(const Layer* layer, compositionengine::LayerFE::LayerSettings& layerSettings);
    void updateMiSecurityDisplayState(uint32_t what, DisplayDeviceState& state,
                const DisplayState& s, uint32_t &flags);
#endif
#ifdef CURTAIN_ANIM
    void updateCurtainStatus(std::shared_ptr<compositionengine::Output> output);
    bool enableCurtainAnim(bool isEnable);
    bool setCurtainAnimRate(float rate);
    void getCurtainStatus(compositionengine::Output* output,
                renderengine::DisplaySettings* displaySettings);
    void updateCurtainAnimClientComposition(bool* forceClientComposition);
    bool mEnableCurtainAnim = false;
    float mCurtainAnimRate = -1.f;
#endif
#ifdef MI_SF_FEATURE
    bool setShadowType(int shadowType, Layer* layer);
    bool setShadowLength(float length, Layer* layer);
    bool setShadowColor(const half4& color, Layer* layer);
    bool setShadowOffset(float offsetX, float offsetY, Layer* layer);
    bool setShadowOutset(float outset, Layer* layer);
    bool setShadowLayers(int32_t numOfLayers, Layer* layer);
    void prepareMiuiShadowClientComposition(
                compositionengine::LayerFE::LayerSettings& caster,
                const Rect& layerStackRect, Layer* layer);
    void setShadowClientStateLocked(uint64_t what,sp<Layer>& layer,
                uint32_t* flags, const layer_state_t& s);
    // HDR Dimmer
    bool mHdrDimmerSupported = false;
    bool getHdrDimmerSupported() { return mHdrDimmerSupported; };
    bool mHdrDimmerEnabled = false;
    float mHdrDimmerFactor = 1.f;
    void updateHdrDimmerState(std::shared_ptr<compositionengine::Output> output,
                              int colorScheme);
    bool enableHdrDimmer(bool enable, float factor);
    void getHdrDimmerState(compositionengine::Output* output,
                renderengine::DisplaySettings* displaySettings);
    bool getHdrDimmerClientComposition();
    // END
#endif

#ifdef QTI_DISPLAY_CONFIG_ENABLED
    int enableQsync(int qsyncMode, String8 pkgName, int pkgSetFps, int qsyncMinFps, int qsyncBaseTiming, int desiredVsync) override;
    bool isQsyncEnabled() override;
#endif

    // MIUI ADD: Added just for Global HBM Dither (J2S-T)
    bool isGlobalHBMDitherEnabled();
    // MIUI ADD: END

    // Filter unnecessary InputWindowInfo
    void setFilterInputFlags(sp<BufferStateLayer> layer);
    bool filterUnnecessaryInput(const BufferLayer* layer);
    void initBlackListForFilterInput();
    // dynamic Surfaceflinger Log
    void setDynamicSfLog(bool dynamic);
    bool isDynamicSfLog();

protected:
    virtual ~MiSurfaceFlingerImpl();

private:
    enum {
        MI_FLINGER_LAYER_FOD_HBM_OVERLAY = 0x1000000,
        MI_FLINGER_LAYER_FOD_ICON = 0x2000000,
        MI_FLINGER_LAYER_AOD_LAYER = 0x4000000,
        MI_FLINGER_LAYER_FOD_ANIM = 0x8000000,
        MI_FLINGER_LAYER_MAX,
    };

    friend class BacklightMonitorThread;
    friend class FodMonitorThread;

    int mFodSystemBacklight;
    int mTargetBrightness;
    int mBrightnessBackup;
    int mMaxBrightness;
    Mutex mFogFlagMutex;
    int mFodFlag = 0;
    bool mIsFodFingerPrint = false;
    bool mLocalHbmEnabled = false;
    uint32_t mIsCastMode = 0;
    uint32_t mIsLastFrame = 0;
    bool mSmartFpsEnable = false;
    bool mFodPowerStateRollOver = false;
    // Flag whether displayfeature support dolby vision.
    bool mDFSupportDV = false;
    bool mGalleryP3DVOpt = false;

    sp<SurfaceFlinger> mFlinger;
    String8 mSaveSurfaceName;
    bool mPreLauncher = false;
    int mPanelId = 0;
    int mFingerprintStatus = 0;
    sp<FodMonitorThread> mFodMonitorThread;
    bool mDozeStatePropertySet = false;
    bool mBlockLauncher = false;
    sp<BacklightMonitorThread> mBacklightMonitorThread;
    unsigned long mDozeState;
    bool mIsAodDetectEnable = false;
    bool mPreAodLayer = false;

    bool mSupportFodneed = false;
    bool mPlatform_8250 = false;
    bool mFakeGcp = false;
    Mutex mGamePackageNameMutex;
    std::string mGamePackageName = std::string("gamelayerdefault");
    int mGameColorMode = LAYER_UNKNOWN;
    DisplayColorSetting mDisplayColorSettingBackup = DisplayColorSetting::kEnhanced;


    bool mPreVideoFullScreen = false;
    bool mPreHasColorFade = false;
    bool mPreWallPaper = false;
    bool mPreIconVisiable = false;
    bool mPreHasHbmOverlay = false;
    bool mPreHasGxzwAnim = false;
    hal::PowerMode mPrePowerMode;
    sp<TouchMonitorThread> mTouchMonitorThread;
    sp<PenMonitorThread> mPenMonitorThread;
    sp<FPMessageThread> mFPMessageThread;
    sp<FingerprintStateManager> mFingerprintStateManager;
    bool mFodEngineEnabled = false;

    std::unordered_map<android::HalDisplayId, mat4> mCurrentColorTransformMap;

    bool mIsDualBuitInDisp = false;
    int mFoldStatus = 0;

    bool mPaperContrastOpt = false;
    bool mSetPCAlpha = false;
    bool mCheckCupHbmCoverlayerEnabled = true;
    int mlastDisplayOritation = 0;
    int mDisplayOritation = 0;

    int mPreviousFrameAffinityMode = -1;
    int mUIAffinityCpuBegin = -1;
    int mUIAffinityCpuEnd = -1;
    int mResetUIAffinityCpuBegin = -1;
    int mResetUIAffinityCpuEnd = -1;
    int mREAffinityCpuBegin = -1;
    int mREAffinityCpuEnd = -1;
    bool getAffinityLimits(const char* affinityProp, const char* defValue,
        int& cpuBegin, int& cpuEnd);
    bool setAffinityImpl(const int& cpuBegin, const int& cpuEnd,
        const int& threadId);
    int getCPUMaxFreq(const char* filepath);
    int getCPUCombType();
    // MIUI+ send SecureWin state
    sp<IBinder> mSecureProxy = nullptr;
    bool mSecondRefreshRateOverlayEnabled = 0;
    // MIUI ADD: Activity Embedding
    bool mEmbeddedEnable = 0;
    bool mHasP3Layers = false;
    bool mDolbyVisionState = false;
#ifdef QTI_DISPLAY_CONFIG_ENABLED
    sp<QsyncFeature> mQsyncFeature = nullptr;
#endif

    // MIUI ADD: Added just for Global HBM Dither (J2S-T)
    bool mGlobalHBMDitherEnabled = false;
    // MIUI ADD: END
    // Filter unnecessary InputWindowInfo
    std::list<std::string> mFilterInputList;
    // dynamic SurfaceFlinger Log
    bool mPropDynamicLog = false;
    bool mDynamicLog = false;
};

class DisplayFeatureCallback : public IDisplayFeatureCallback
{
public:
    typedef void (*callback_t)(void *user, int event, int value, float r, float g, float b);
    DisplayFeatureCallback(callback_t cb, void *user);
    Return<void> displayfeatureInfoChanged(int32_t caseId, int32_t value, float red, float green, float blue);
private:
    callback_t mCbf;
    void *mUser;
};

}
#endif
