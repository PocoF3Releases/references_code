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


#ifndef ANDROID_IQCOM_SURFACEFLINGER_IMPL_H
#define ANDROID_IQCOM_SURFACEFLINGER_IMPL_H

#include "SurfaceFlinger.h"
#include <compositionengine/CompositionRefreshArgs.h>
#include <compositionengine/Output.h>
#include <Scheduler/MessageQueue.h>
#include <compositionengine/impl/Output.h>
#include <compositionengine/LayerFE.h>
#include <gui/WindowInfo.h>
#include "Layer.h"
#include "BufferStateLayer.h"
#include <ui/DisplayId.h>
#include <gui/IMiSurfaceComposerStub.h>
#include <math/vec4.h>
namespace android {
using DisplayColorSetting = compositionengine::OutputColorSetting;
using Output = compositionengine::impl::Output;

class IMiSurfaceFlingerStub {
public:
    IMiSurfaceFlingerStub() {}
    virtual ~IMiSurfaceFlingerStub() {}

    virtual int init(sp<SurfaceFlinger> flinger);

    virtual void checkLayerStack();

    virtual void queryPrimaryDisplayLayersInfo(const std::shared_ptr<android::compositionengine::Output> output,
                                       uint32_t& layersInfo);

    virtual bool hookMiSetColorTransform(const compositionengine::CompositionRefreshArgs& args,
                                         const std::optional<android::HalDisplayId> halDisplayId,
                                         bool isDisconnected);

    virtual void updateFodFlag(int32_t flag);

    virtual void updateUnsetFps(int32_t flag);

    virtual void updateLHBMFodFlag(int32_t flag);

    virtual void updateMiFodFlag(compositionengine::Output *output,
                                 const compositionengine::CompositionRefreshArgs& refreshArgs);

    virtual void updateSystemUIFlag(int32_t flag);

    virtual void saveFodSystemBacklight(int backlight);

    virtual void prePowerModeFOD(hal::PowerMode mode);

    virtual void postPowerModeFOD(hal::PowerMode mode);

    virtual void notifySaveSurface(String8 name);

    virtual void lowBrightnessFOD(int32_t state);

    virtual void writeFPStatusToKernel(int fingerprintStatus, int lowBrightnessFOD);

    virtual bool validateLayerSize(uint32_t w, uint32_t h);

    virtual void getHBMLayerAlpha(Layer* layer, float& alpha);

    virtual void setGameArgs(compositionengine::CompositionRefreshArgs& refreshArgs);

    virtual void saveDisplayColorSetting(DisplayColorSetting displayColorSetting);

    virtual void updateGameColorSetting(String8 gamePackageName, int gameColorMode, DisplayColorSetting& mDisplayColorSetting);

    virtual void updateForceSkipIdleTimer(int mode, bool isPrimary);
    // BSP-Game: add Game Fps Stat
    virtual sp<Layer> getGameLayer(const char* activityName);

    virtual bool isFpsStatSupported(Layer* layer);

    virtual void createFpsStat(Layer* layer);

    virtual void destroyFpsStat(Layer* layer);

    virtual void insertFrameTime(Layer* layer, nsecs_t time);

    virtual void insertFrameFenceTime(Layer* layer, const std::shared_ptr<FenceTime>& fence);

    virtual bool isFpsStatEnabled(Layer* layer);

    virtual void enableFpsStat(Layer* layer, bool enable);

    virtual void setBadFpsStandards(Layer* layer, int targetFps, int thresh1, int thresh2);

    virtual void reportFpsStat(Layer* layer, Parcel* reply);

    virtual bool isFpsMonitorRunning(Layer* layer);

    virtual void startFpsMonitor(Layer* layer, int timeout);

    virtual void stopFpsMonitor(Layer* layer);
    // END
    virtual void launcherUpdate(Layer* layer);

    virtual bool allowAospVrr();

    virtual bool isModeChangeOpt();

    virtual void addDolbyVisionHdr(std::vector<ui::Hdr> &hdrTypes);

    virtual void signalLayerUpdate(uint32_t scense, sp<Layer> layer);

    virtual void disableOutSupport(bool* outSupport);

    virtual status_t enableSmartDfps(int32_t fps);

    virtual void updateScene(const int& module, const int& value, const String8& pkg);

    virtual status_t updateDisplayLockFps(int32_t lockFps, bool ddicAutoMode = false, int ddicMinFps = 0);

    virtual status_t updateGameFlag(int32_t flag);

    virtual status_t updateVideoInfo(s_VideoInfo *videoInfo);

    virtual status_t updateVideoDecoderInfo(s_VideoDecoderInfo *videoDecoderInfo);

    virtual void updateFpsRangeByLTPO(DisplayDevice* displayDevice, FpsRange* fpsRange);

    virtual void setDVStatus(int32_t enable);

    virtual bool isLtpoPanel();

    virtual bool allowCtsPass(std::string name);

    virtual bool isResetIdleTimer();

    virtual bool isVideoScene();

    virtual bool isForceSkipIdleTimer();

    virtual bool isMinIdleFps(nsecs_t period);

    virtual bool isTpIdleScene(int32_t current_setting_fps);

    virtual bool isSceneExist(scene_module_t scene_module);

    virtual nsecs_t setMinIdleFpsPeriod(nsecs_t period);

    virtual bool isDdicIdleModeGroup(int32_t modeGroup);

    virtual bool isDdicAutoModeGroup(int32_t modeGroup);

    virtual bool isDdicQsyncModeGroup(int32_t modeGroup);

    virtual bool isNormalModeGroup(int32_t modeGroup);

    virtual uint32_t getDdicMinFpsByGroup(int32_t modeGroup);

    virtual int32_t getGroupByBrightness(int32_t fps, int32_t modeGroup);

    virtual DisplayModePtr setIdleFps(int32_t mode_group, bool *minFps);

    virtual DisplayModePtr setVideoFps(int32_t mode_group);

    virtual DisplayModePtr setTpIdleFps(int32_t mode_group);

    virtual DisplayModePtr setPromotionModeId(int32_t defaultMode);

    virtual void setLayerRealUpdateFlag(uint32_t scense, bool flag);

    virtual bool isCallingBySystemui(const int callingPid);

    virtual status_t setDesiredMultiDisplayPolicy(const sp<DisplayDevice>& display,
            const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy);

    virtual status_t setDesiredMultiDisplayModeSpecsInternal(const sp<DisplayDevice>& display,
            const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy);

    virtual void setupIdleTimeoutHandle(uint32_t hwcDisplayId, bool enableDynamicIdle, bool isPrimary);

    virtual void dumpDisplayFeature(std::string& result);

    virtual void setOrientationStatustoDF();

    virtual void saveDefaultResolution(DisplayModes supportedModes);

    virtual void setAffinity(const int& affinityMode);
    virtual void setreAffinity(const int& reTid);

#if MI_VIRTUAL_DISPLAY_FRAMERATE
    virtual bool skipComposite(const sp<DisplayDevice>& display);

    virtual void setLimitedFrameRate(const sp<DisplayDevice>& display, const DisplayDeviceState& state);

    virtual void updateLimitedFrameRate(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState);

    virtual void updateDisplayStateLimitedFrameRate(uint32_t what, DisplayDeviceState& state,
            const DisplayState& s, uint32_t &flags);

    virtual void initLimitedFrameRate(DisplayState& d);

    virtual void setLimitedFrameRateDisplayDevice(DisplayDevice * display, uint32_t frameRate);

    virtual bool needToCompositeDisplayDevice(DisplayDevice * display);
#endif

    virtual void  setFrameBufferSizeforResolutionChange(sp<DisplayDevice> displayDevice);

#if MI_SCREEN_PROJECTION
    virtual void setDiffScreenProjection(uint32_t isScreenProjection, Output* outPut);

    virtual void setLastFrame(uint32_t isLastFrame, Output* outPut);

    virtual void setCastMode(uint32_t isCastMode, Output* outPut);

    virtual bool isNeedSkipPresent(Output* outPut);

    virtual void rebuildLayerStacks(Output* outPut);

    virtual void ensureOutputLayerIfVisible(Output* outPut, const compositionengine::LayerFECompositionState* layerFEState);

    virtual bool includesLayer(const sp<compositionengine::LayerFE>& layerFE,const Output* outPut);

    virtual bool continueComputeBounds(Layer* layer);

    virtual void prepareBasicGeometryCompositionState(
              compositionengine::LayerFECompositionState* compositionState, const Layer::State& drawingState, Layer* layer);

    virtual bool setScreenFlags(int32_t screenFlags, Layer* layer);

    virtual void setChildImeFlags(uint32_t flags, uint32_t mask, Layer* layer);

    virtual String8 getLayerProtoName(Layer* layer, const Layer::State& state);

    virtual void fillInputInfo(gui::WindowInfo* info, Layer* layer);

    virtual uint32_t isCastMode();

    virtual void setIsCastMode(uint32_t castMode);

    virtual uint32_t isLastFrame();

    virtual void setIsLastFrame(uint32_t lastFrame);

    virtual void setupNewDisplayDeviceInternal(sp<DisplayDevice>& display, const DisplayDeviceState& state);

    virtual void applyMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays, uint32_t* flags);

    virtual uint32_t setMiuiDisplayStateLocked(const MiuiDisplayState& s);

    virtual void setClientStateLocked(uint64_t what,sp<Layer>& layer,uint32_t* flags, const layer_state_t& s);

    virtual String8 getDumpsysInfo();

    virtual bool showContineTraverseLayersInLayerStack(const sp<Layer>& layer);

    virtual void setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays, uint32_t flags,
                                             int64_t desiredPresentTime);
#endif

    // MIUI+ send SecureWin state
    virtual void setSecureProxy(sp<IBinder> secureProxy);

    virtual sp<IBinder> getSecureProxy();

    virtual void updateOutputExtra(std::shared_ptr<compositionengine::Output> output, bool isInternalDisplay, int sequenceId);

    virtual void handleSecureProxy(compositionengine::Output* output, uint32_t layerStackId);
    // END

#ifdef MI_SF_FEATURE
    virtual void setNewMiSecurityDisplayState(sp<DisplayDevice>& display, const DisplayDeviceState& state);
    virtual void setMiSecurityDisplayState(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState);
    virtual bool getMiSecurityDisplayState(const compositionengine::Output* output);
    virtual void setVirtualDisplayCoverBlur(const Layer* layer, compositionengine::LayerFE::LayerSettings& layerSettings);
    virtual void updateMiSecurityDisplayState(uint32_t what, DisplayDeviceState& state,
                const DisplayState& s, uint32_t &flags);
#endif
#ifdef CURTAIN_ANIM
    virtual void updateCurtainStatus(std::shared_ptr<compositionengine::Output> output);
    virtual bool enableCurtainAnim(bool isEnable);
    virtual bool setCurtainAnimRate(float rate);
    virtual void getCurtainStatus(compositionengine::Output* output,
                                  renderengine::DisplaySettings* displaySettings);
    virtual void updateCurtainAnimClientComposition(bool* forceClientComposition);
#endif
#ifdef MI_SF_FEATURE
    virtual bool setShadowType(int shadowType, Layer* layer);
    virtual bool setShadowLength(float length, Layer* layer);
    virtual bool setShadowColor(const half4& color, Layer* layer);
    virtual bool setShadowOffset(float offsetX, float offsetY, Layer* layer);
    virtual bool setShadowOutset(float outset, Layer* layer);
    virtual bool setShadowLayers(int32_t numOfLayers, Layer* layer);
    virtual void prepareMiuiShadowClientComposition(
                        compositionengine::LayerFE::LayerSettings& caster,
                        const Rect& layerStackRect, Layer* layer);
    virtual void setShadowClientStateLocked(uint64_t what,sp<Layer>& layer,
                        uint32_t* flags, const layer_state_t& s);
   // MIUI ADD: HDR Dimmer
    virtual bool getHdrDimmerSupported();
    virtual void updateHdrDimmerState(std::shared_ptr<compositionengine::Output> output, int colorScheme);
    virtual bool enableHdrDimmer(bool enable, float factor);
    virtual void getHdrDimmerState(compositionengine::Output* output,
                                  renderengine::DisplaySettings* displaySettings);
    virtual bool getHdrDimmerClientComposition();
    // END
#endif
    virtual bool isFold();
    virtual bool isDualBuitinDisp();
    virtual bool isCTS();
    virtual sp<DisplayDevice> getActiveDisplayDevice();
    virtual bool isSecondRefreshRateOverlayEnabled();
    virtual bool isEmbeddedEnable();
    virtual void changeLayerDataSpace(Layer* layer);

#ifdef QTI_DISPLAY_CONFIG_ENABLED
    virtual int enableQsync(int qsyncMode, String8 pkgName, int pkgSetFps, int qsyncMinFps, int qsyncBaseTiming, int desiredVsync);
    virtual bool isQsyncEnabled();
#endif

    // MIUI ADD: Added just for Global HBM Dither (J2S-T)
    virtual bool isGlobalHBMDitherEnabled();
    // MIUI ADD: END

    //for dual-screen boot animation
    virtual void  initSecondaryDisplayLocked(Vector<DisplayState>& displays);

    // Filter unnecessary InputWindowInfo
    virtual void setFilterInputFlags(sp<BufferStateLayer> layer);
    virtual bool filterUnnecessaryInput(const BufferLayer* layer);

    // dynamic SurfaceFlinger Log
    virtual void setDynamicSfLog(bool dynamic);
    virtual bool isDynamicSfLog();
};

typedef IMiSurfaceFlingerStub* Create();
typedef void Destroy(IMiSurfaceFlingerStub *);

} // namespace android

#endif
