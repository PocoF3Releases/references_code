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

#pragma once
#include "IMiSurfaceFlingerStub.h"
#include <mutex>
#include <compositionengine/impl/Output.h>
#include <compositionengine/LayerFE.h>
#include <gui/WindowInfo.h>
#include "Layer.h"
#include <gui/IMiSurfaceComposerStub.h>

namespace android {
//question: can't remove
class IMiSurfaceFlingerStub;
using Output = compositionengine::impl::Output;
class MiSurfaceFlingerStub {

private:

    MiSurfaceFlingerStub() {}
    static void *LibMiSurfaceFlingerImpl;
    static IMiSurfaceFlingerStub *ImplInstance;
    static IMiSurfaceFlingerStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

    static constexpr const char* LIBPATH = "/system_ext/lib64/libmisurfaceflinger.so";

public:

    enum {
        MI_FPS_MIN_1HZ = 1,
        MI_FPS_MIN_10HZ = 10,
        MI_FPS_MIN_30HZ = 30,
    };

    virtual ~MiSurfaceFlingerStub() {}

    static int init(sp<SurfaceFlinger> flinger);

    static void checkLayerStack();

    static void queryPrimaryDisplayLayersInfo(const std::shared_ptr<android::compositionengine::Output> output,
                                      uint32_t& layersInfo);

    static bool hookMiSetColorTransform(const compositionengine::CompositionRefreshArgs& args,
                                        const std::optional<android::HalDisplayId> halDisplayId,
                                        bool isDisconnected);

    static void updateMiFodFlag(compositionengine::Output *output,
                                const compositionengine::CompositionRefreshArgs& refreshArgs);

    static void updateFodFlag(int32_t flag);

    static void updateUnsetFps(int32_t flag);

    static void updateLHBMFodFlag(int32_t flag);

    static void updateSystemUIFlag(int32_t flag);

    static void saveFodSystemBacklight(int32_t backlight);

    static void prePowerModeFOD(hal::PowerMode mode);

    static void postPowerModeFOD(hal::PowerMode mode);

    static void notifySaveSurface(String8 name);

    static void lowBrightnessFOD(int32_t state);

    static void writeFPStatusToKernel(int fingerprintStatus, int lowBrightnessFOD);

    static bool validateLayerSize(uint32_t w, uint32_t h);

    static void getHBMLayerAlpha(Layer* layer, float& alpha);

    static void setGameArgs(compositionengine::CompositionRefreshArgs& refreshArgs);

    static void saveDisplayColorSetting(DisplayColorSetting displayColorSetting);

    static void updateGameColorSetting(String8 gamePackageName, int gameColorMode, DisplayColorSetting& mDisplayColorSetting);

    static void updateForceSkipIdleTimer(int mode, bool isPrimary);

    static void launcherUpdate(Layer* layer);

    static bool allowAospVrr();

    static bool isModeChangeOpt();

    static void addDolbyVisionHdr(std::vector<ui::Hdr> &hdrTypes);

    static bool isLtpoPanel();

    static bool allowCtsPass(std::string name);

    static bool isResetIdleTimer();

    static bool isVideoScene();

    static bool isForceSkipIdleTimer();

    static bool isMinIdleFps(nsecs_t period);

    static bool isTpIdleScene(int32_t current_setting_fps);

    static bool isSceneExist(scene_module_t scene_module);

    static nsecs_t setMinIdleFpsPeriod(nsecs_t period);

    static bool isDdicIdleModeGroup(int32_t modeGroup);

    static bool isDdicAutoModeGroup(int32_t modeGroup);

    static bool isDdicQsyncModeGroup(int32_t modeGroup);

    static bool isNormalModeGroup(int32_t modeGroup);

    static uint32_t getDdicMinFpsByGroup(int32_t modeGroup);

    static int32_t getGroupByBrightness(int32_t fps, int32_t modeGroup);

    static DisplayModePtr setIdleFps(int32_t mode_group, bool *minFps);

    static DisplayModePtr setVideoFps(int32_t mode_group);

    static DisplayModePtr setTpIdleFps(int32_t mode_group);

    static DisplayModePtr setPromotionModeId(int32_t defaultMode);

    static void updateScene(const int& module, const int& value, const String8& pkg);

    static void setLayerRealUpdateFlag(uint32_t scense, bool flag);

    static status_t updateDisplayLockFps(int32_t lockFps, bool ddicAutoMode = false, int ddicMinFps = 0);

    static status_t updateGameFlag(int32_t flag);

    static status_t updateVideoInfo(s_VideoInfo *videoInfo);

    static status_t updateVideoDecoderInfo(s_VideoDecoderInfo *videoDecoderInfo);

    static void updateFpsRangeByLTPO(DisplayDevice* displayDevice, FpsRange* fpsRange);

    static void setDVStatus(int32_t enable);

    static void signalLayerUpdate(uint32_t scense, sp<Layer> layer);

    static void disableOutSupport(bool* outSupport);

    static status_t enableSmartDfps(int32_t fps);

    static bool isCallingBySystemui(const int callingPid);

    static status_t setDesiredMultiDisplayPolicy(const sp<DisplayDevice>& display,
            const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy);

    static status_t setDesiredMultiDisplayModeSpecsInternal(const sp<DisplayDevice>& display,
        const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy);

    static void setupIdleTimeoutHandle(uint32_t hwcDisplayId, bool enableDynamicIdle, bool isPrimary);

    static void dumpDisplayFeature(std::string& result);
    static void setOrientationStatustoDF();
    static void saveDefaultResolution(DisplayModes supportedModes);

    static void setAffinity(const int& affinityMode);
    static void setreAffinity(const int& reTid);
    // BSP-Game: add Game Fps Stat

#if MI_VIRTUAL_DISPLAY_FRAMERATE
    static bool skipComposite(const sp<DisplayDevice>& display);

    static void setLimitedFrameRate(const sp<DisplayDevice>& display, const DisplayDeviceState& state);

    static void updateLimitedFrameRate(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState);

    static void updateDisplayStateLimitedFrameRate(uint32_t what, DisplayDeviceState& state,
            const DisplayState& s, uint32_t &flags);

    static void initLimitedFrameRate(DisplayState& d);

    static void setLimitedFrameRateDisplayDevice(DisplayDevice * display, uint32_t frameRate);

    static bool needToCompositeDisplayDevice(DisplayDevice * display);
#endif

    static sp<Layer> getGameLayer(const char* activityName);

    static bool isFpsStatSupported(Layer* layer);

    static void createFpsStat(Layer* layer);

    static void destroyFpsStat(Layer* layer);

    static void insertFrameTime(Layer* layer, nsecs_t time);

    static void insertFrameFenceTime(Layer* layer, const std::shared_ptr<FenceTime>& fence);

    static bool isFpsStatEnabled(Layer* layer);

    static void enableFpsStat(Layer* layer, bool enable);

    static void setBadFpsStandards(Layer* layer, int targetFps, int thresh1, int thresh2);

    static void reportFpsStat(Layer* layer, Parcel* reply);

    static bool isFpsMonitorRunning(Layer* layer);

    static void startFpsMonitor(Layer* layer, int timeout);

    static void stopFpsMonitor(Layer* layer);
    // END
    static void  setFrameBufferSizeforResolutionChange(sp<DisplayDevice> displayDevice);

    static void setDiffScreenProjection(uint32_t isScreenProjection, Output* outPut);

    static void setLastFrame(uint32_t isLastFrame, Output* outPut);

    static void setCastMode(uint32_t isCastMode, Output* outPut);

    static bool isNeedSkipPresent(Output* outPut);

    static void rebuildLayerStacks(Output* outPut);

    static void ensureOutputLayerIfVisible(Output* outPut, const compositionengine::LayerFECompositionState* layerFEState);

    static bool includesLayer(const sp<compositionengine::LayerFE>& layerFE, const Output* outPut);

    static bool continueComputeBounds(Layer* layer);

    static void prepareBasicGeometryCompositionState(
                  compositionengine::LayerFECompositionState* compositionState, const Layer::State& drawingState, Layer* layer);

    static bool setScreenFlags(int32_t screenFlags, Layer* layer);

    static void setChildImeFlags(uint32_t flags, uint32_t mask, Layer* layer);

    static String8 getLayerProtoName(Layer* layer, const Layer::State& state);

    static void fillInputInfo(gui::WindowInfo* info, Layer* layer);

    static uint32_t isCastMode();

    static void setIsCastMode(uint32_t castMode);

    static uint32_t isLastFrame();

    static void setIsLastFrame(uint32_t lastFrame);

    static void setupNewDisplayDeviceInternal(sp<DisplayDevice>& display, const DisplayDeviceState& state);

    static void setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays, uint32_t flags,
                                                 int64_t desiredPresentTime);

    static void applyMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays, uint32_t* flags);

    static uint32_t setMiuiDisplayStateLocked(const MiuiDisplayState& s);

    static void setClientStateLocked(uint64_t what,sp<Layer>& layer,uint32_t* flags, const layer_state_t& s);

    static String8 getDumpsysInfo();

    static bool showContineTraverseLayersInLayerStack(const sp<Layer>& layer);

    // MIUI+ send SecureWin state
    static void setSecureProxy(sp<IBinder> secureProxy);

    static sp<IBinder> getSecureProxy();

    static void updateOutputExtra(std::shared_ptr<compositionengine::Output> output, bool isPrimaryDisplay, int sequenceId);

    static void handleSecureProxy(compositionengine::Output* output, uint32_t layerStackId);
    // END
#ifdef MI_SF_FEATURE
    static void setNewMiSecurityDisplayState(sp<DisplayDevice>& display, const DisplayDeviceState& state);
    static void setMiSecurityDisplayState(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState);
    static bool getMiSecurityDisplayState(const compositionengine::Output* output);
    static void setVirtualDisplayCoverBlur(const Layer* layer, compositionengine::LayerFE::LayerSettings& layerSettings);
    static void updateMiSecurityDisplayState(uint32_t what, DisplayDeviceState& state,
                const DisplayState& s, uint32_t &flags);
#endif

#ifdef CURTAIN_ANIM
    static void updateCurtainStatus(std::shared_ptr<compositionengine::Output> output);
    static bool enableCurtainAnim(bool isEnable);
    static bool setCurtainAnimRate(float rate);
    static void getCurtainStatus(compositionengine::Output* output,
                                 renderengine::DisplaySettings* displaySettings);
    static void updateCurtainAnimClientComposition(bool* forceClientComposition);
#endif

#ifdef MI_SF_FEATURE
    static bool setShadowType(int shadowType, Layer* layer);
    static bool setShadowLength(float length, Layer* layer);
    static bool setShadowColor(const half4& color, Layer* layer);
    static bool setShadowOffset(float offsetX, float offsetY, Layer* layer);
    static bool setShadowOutset(float outset, Layer* layer);
    static bool setShadowLayers(int32_t numOfLayers, Layer* layer);
    static void prepareMiuiShadowClientComposition(
                        compositionengine::LayerFE::LayerSettings& caster,
                        const Rect& layerStackRect, Layer* layer);
    static void setShadowClientStateLocked(uint64_t what,sp<Layer>& layer,
                        uint32_t* flags, const layer_state_t& s);
    // MIUI ADD: HDR Dimmer
    static bool getHdrDimmerSupported();
    static void updateHdrDimmerState(std::shared_ptr<compositionengine::Output> output, int colorScheme);
    static bool enableHdrDimmer(bool enable, float factor);
    static void getHdrDimmerState(compositionengine::Output* output,
                                 renderengine::DisplaySettings* displaySettings);
    static bool getHdrDimmerClientComposition();
    // END
#endif

    static bool isFold();
    static bool isDualBuitinDisp();
    static bool isCTS();
    static sp<DisplayDevice> getActiveDisplayDevice();
    static bool isSecondRefreshRateOverlayEnabled();
    static bool isEmbeddedEnable();
    static void changeLayerDataSpace(Layer* layer);

#ifdef QTI_DISPLAY_CONFIG_ENABLED
    static int enableQsync(int qsyncMode, String8 pkgName = String8("Unknown"), int pkgSetFps = 0, int qsyncMinFps = 0, int qsyncBaseTiming = 0, int desiredVsync = 0);
    static bool isQsyncEnabled();
#endif

    // MIUI ADD: Added just for Global HBM Dither (J2S-T)
    static bool isGlobalHBMDitherEnabled();
    // MIUI ADD: END
    // Filter unnecessary InputWindowInfo
    static void setFilterInputFlags(sp<BufferStateLayer> layer);
    static bool filterUnnecessaryInput(const BufferLayer* layer);
    // END

    //for dual-screen boot animation
    static void initSecondaryDisplayLocked(Vector<DisplayState>& displays);

    // dynamic SurfaceFlinger Log
    static void setDynamicSfLog(bool dynamic);
    static bool isDynamicSfLog();
    // END
};


} // namespace android
