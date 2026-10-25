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

#include "MiSurfaceFlingerStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

//#define LOG_NDEBUG 0

namespace android {

void* MiSurfaceFlingerStub::LibMiSurfaceFlingerImpl = nullptr;
IMiSurfaceFlingerStub* MiSurfaceFlingerStub::ImplInstance = nullptr;
std::mutex MiSurfaceFlingerStub::StubLock;

IMiSurfaceFlingerStub* MiSurfaceFlingerStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiSurfaceFlingerImpl) {
        LibMiSurfaceFlingerImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiSurfaceFlingerImpl) {
            Create* create = (Create *)dlsym(LibMiSurfaceFlingerImpl, "create");
            ImplInstance = create();
        } else {
            static bool onlyOnceOutput = true;
            if (onlyOnceOutput) {
                ALOGE("open %s failed! dlopen - %s", LIBPATH, dlerror());
                onlyOnceOutput = false;
            }
        }
    }
    return ImplInstance;
}

void MiSurfaceFlingerStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiSurfaceFlingerImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibMiSurfaceFlingerImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibMiSurfaceFlingerImpl);
        LibMiSurfaceFlingerImpl = nullptr;
        ImplInstance = nullptr;
    }
}

int MiSurfaceFlingerStub::init(sp<SurfaceFlinger> flinger) {
    int ret = 0;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->init(flinger);
    }
    return ret;
}

void MiSurfaceFlingerStub::queryPrimaryDisplayLayersInfo(const std::shared_ptr<android::compositionengine::Output> output,
                                        uint32_t& layersInfo) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->queryPrimaryDisplayLayersInfo(output, layersInfo);
    }
}

bool MiSurfaceFlingerStub::hookMiSetColorTransform(const compositionengine::CompositionRefreshArgs& args,
                                                   const std::optional<android::HalDisplayId> halDisplayId,
                                                   bool isDisconnected) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->hookMiSetColorTransform(args, halDisplayId, isDisconnected);
    }
    return ret;
}

void MiSurfaceFlingerStub::checkLayerStack() {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->checkLayerStack();
    }
}

void MiSurfaceFlingerStub::updateFodFlag(int32_t flag) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateFodFlag(flag);
    }
}

void MiSurfaceFlingerStub::updateUnsetFps(int32_t flag) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateUnsetFps(flag);
    }
}

void MiSurfaceFlingerStub::updateLHBMFodFlag(int32_t flag) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateLHBMFodFlag(flag);
    }
}

void MiSurfaceFlingerStub::updateMiFodFlag(compositionengine::Output *output,
                                           const compositionengine::CompositionRefreshArgs& refreshArgs) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateMiFodFlag(output, refreshArgs);
    }
}

void MiSurfaceFlingerStub::updateSystemUIFlag(int32_t flag) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateSystemUIFlag(flag);
    }
}

void MiSurfaceFlingerStub::saveFodSystemBacklight(int32_t backlight) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->saveFodSystemBacklight(backlight);
    }
}

void MiSurfaceFlingerStub::prePowerModeFOD(hal::PowerMode mode) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->prePowerModeFOD(mode);
    }
}

void MiSurfaceFlingerStub::postPowerModeFOD(hal::PowerMode mode) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->postPowerModeFOD(mode);
    }
}

void MiSurfaceFlingerStub::notifySaveSurface(String8 name) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->notifySaveSurface(name);
    }
}

void MiSurfaceFlingerStub::lowBrightnessFOD(int32_t state) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->lowBrightnessFOD(state);
    }
}

void MiSurfaceFlingerStub::writeFPStatusToKernel(int fingerPrintStatus,
                                                 int lowBrightnessFOD) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->writeFPStatusToKernel(fingerPrintStatus, lowBrightnessFOD);
    }
}

bool MiSurfaceFlingerStub::validateLayerSize(uint32_t w, uint32_t h) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->validateLayerSize(w, h);
    }
    return ret;
}

void MiSurfaceFlingerStub::getHBMLayerAlpha(Layer* layer, float& alpha) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->getHBMLayerAlpha(layer, alpha);
    }
}

void MiSurfaceFlingerStub::setGameArgs(compositionengine::CompositionRefreshArgs& refreshArgs){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setGameArgs(refreshArgs);
    }
}

void MiSurfaceFlingerStub::saveDisplayColorSetting(DisplayColorSetting displayColorSetting){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->saveDisplayColorSetting(displayColorSetting);
    }
}

void MiSurfaceFlingerStub::updateGameColorSetting(String8 gamePackageName, int gameColorMode, DisplayColorSetting& mDisplayColorSetting){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateGameColorSetting(gamePackageName, gameColorMode, mDisplayColorSetting);
    }
}

void MiSurfaceFlingerStub::updateForceSkipIdleTimer(int mode, bool isPrimary){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateForceSkipIdleTimer(mode,isPrimary);
    }
}

void MiSurfaceFlingerStub::launcherUpdate(Layer* layer){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->launcherUpdate(layer);
    }
}

void MiSurfaceFlingerStub::addDolbyVisionHdr(std::vector<ui::Hdr> &hdrTypes){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->addDolbyVisionHdr(hdrTypes);
    }
}

bool MiSurfaceFlingerStub::allowAospVrr() {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->allowAospVrr();
    }
    return ret;
}

bool MiSurfaceFlingerStub::isModeChangeOpt() {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isModeChangeOpt();
    }
    return ret;
}

bool MiSurfaceFlingerStub::isForceSkipIdleTimer(){
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isForceSkipIdleTimer();
    }
    return ret;
}

bool MiSurfaceFlingerStub::isMinIdleFps(nsecs_t period) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isMinIdleFps(period);
    }
    return ret;
}

nsecs_t MiSurfaceFlingerStub::setMinIdleFpsPeriod(nsecs_t period) {
    nsecs_t ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isMinIdleFps(period);
    }
    return ret;
}

bool MiSurfaceFlingerStub::isLtpoPanel() {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isLtpoPanel();
    }
    return ret;
}

bool MiSurfaceFlingerStub::isDdicIdleModeGroup(int32_t modeGroup) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isDdicIdleModeGroup(modeGroup);
    }
    return ret;
}

bool MiSurfaceFlingerStub::isDdicAutoModeGroup(int32_t modeGroup) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isDdicAutoModeGroup(modeGroup);
    }
    return ret;
}

bool MiSurfaceFlingerStub::isDdicQsyncModeGroup(int32_t modeGroup) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isDdicQsyncModeGroup(modeGroup);
    }
    return ret;
}


bool MiSurfaceFlingerStub::isNormalModeGroup(int32_t modeGroup) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isNormalModeGroup(modeGroup);
    }
    return ret;
}

uint32_t MiSurfaceFlingerStub::getDdicMinFpsByGroup(int32_t modeGroup) {
    uint32_t ret = 0;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->getDdicMinFpsByGroup(modeGroup);
    }
    return ret;
}
int32_t MiSurfaceFlingerStub::getGroupByBrightness(int32_t fps, int32_t modeGroup) {
    int32_t ret = 0;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->getGroupByBrightness(fps, modeGroup);
    }
    return ret;
}

DisplayModePtr MiSurfaceFlingerStub::setIdleFps(int32_t mode_group, bool *minFps) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    DisplayModePtr ret = nullptr;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->setIdleFps(mode_group, minFps);
    }
    return ret;
}

DisplayModePtr MiSurfaceFlingerStub::setVideoFps(int32_t mode_group) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    DisplayModePtr ret = nullptr;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->setVideoFps(mode_group);
    }
    return ret;
}

DisplayModePtr MiSurfaceFlingerStub::setTpIdleFps(int32_t mode_group) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    DisplayModePtr ret = nullptr;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->setTpIdleFps(mode_group);
    }
    return ret;
}

DisplayModePtr MiSurfaceFlingerStub::setPromotionModeId(int32_t defaultMode) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    DisplayModePtr ret = nullptr;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->setPromotionModeId(defaultMode);
    }
    return ret;
}

bool MiSurfaceFlingerStub::allowCtsPass(std::string name){
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->allowCtsPass(name);
    }
    return ret;
}
bool MiSurfaceFlingerStub::isResetIdleTimer() {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isResetIdleTimer();
    }
    return ret;
}

bool MiSurfaceFlingerStub::isVideoScene() {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isVideoScene();
    }
    return ret;
}

bool MiSurfaceFlingerStub::isTpIdleScene(int32_t current_setting_fps) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isTpIdleScene(current_setting_fps);
    }
    return ret;
}

bool MiSurfaceFlingerStub::isSceneExist(scene_module_t scene_module) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isSceneExist(scene_module);
    }
    return ret;
}

void MiSurfaceFlingerStub::updateScene(const int& module, const int& value, const String8& pkg) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateScene(module, value, pkg);
    }
}

void MiSurfaceFlingerStub::setLayerRealUpdateFlag(uint32_t scense, bool flag) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setLayerRealUpdateFlag(scense, flag);
    }
}

status_t MiSurfaceFlingerStub::updateDisplayLockFps(int32_t lockFps, bool ddicAutoMode, int ddicMinFps) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    status_t res = NO_ERROR;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        res= Istub->updateDisplayLockFps(lockFps, ddicAutoMode, ddicMinFps);
    }
    return res;
}

status_t MiSurfaceFlingerStub::updateGameFlag(int32_t flag) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    status_t res = NO_ERROR;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        res= Istub->updateGameFlag(flag);
    }
    return res;
}

status_t MiSurfaceFlingerStub::updateVideoInfo(s_VideoInfo *videoInfo) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    status_t res = NO_ERROR;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        res= Istub->updateVideoInfo(videoInfo);
    }
    return res;
}

status_t MiSurfaceFlingerStub::updateVideoDecoderInfo(s_VideoDecoderInfo *videoDecoderInfo) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    status_t res = NO_ERROR;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        res= Istub->updateVideoDecoderInfo(videoDecoderInfo);
    }
    return res;
}


void MiSurfaceFlingerStub::updateFpsRangeByLTPO(DisplayDevice* displayDevice, FpsRange* fpsRange) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateFpsRangeByLTPO(displayDevice, fpsRange);
    }
}

void MiSurfaceFlingerStub::setDVStatus(int32_t enable) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setDVStatus(enable);
    }
}

void MiSurfaceFlingerStub::signalLayerUpdate(uint32_t scense, sp<Layer> layer){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->signalLayerUpdate(scense, layer);
    }
}

void MiSurfaceFlingerStub::disableOutSupport(bool* outSupport){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->disableOutSupport(outSupport);
    }
}

status_t MiSurfaceFlingerStub::enableSmartDfps(int32_t fps){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    status_t res = NO_ERROR;
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        res= Istub->enableSmartDfps(fps);
    }
    return res;
}

bool MiSurfaceFlingerStub::isCallingBySystemui(const int callingPid) {
    bool ret = false;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->isCallingBySystemui(callingPid);
    }
    return ret;
}

status_t MiSurfaceFlingerStub::setDesiredMultiDisplayPolicy(const sp<DisplayDevice>& display,
            const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy) {
     IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setDesiredMultiDisplayPolicy(display, policy, overridePolicy);
    }
    return BAD_VALUE;
}

status_t MiSurfaceFlingerStub::setDesiredMultiDisplayModeSpecsInternal(const sp<DisplayDevice>& display,
        const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy)
{
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setDesiredMultiDisplayModeSpecsInternal(display, policy, overridePolicy);
    }
    return BAD_VALUE;
}

void MiSurfaceFlingerStub::setupIdleTimeoutHandle(uint32_t hwcDisplayId, bool enableDynamicIdle, bool isPrimary) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setupIdleTimeoutHandle(hwcDisplayId, enableDynamicIdle, isPrimary);
    }
}

// BSP-Game: add Game Fps Stat
sp<Layer> MiSurfaceFlingerStub::getGameLayer(const char* activityName) {
    sp<Layer> spl;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        return Istub->getGameLayer(activityName);
    return spl;
}

bool MiSurfaceFlingerStub::isFpsStatSupported(Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        return Istub->isFpsStatSupported(layer);
    return false;
}

void MiSurfaceFlingerStub::createFpsStat(Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->createFpsStat(layer);
}

void MiSurfaceFlingerStub::destroyFpsStat(Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->destroyFpsStat(layer);
}

void MiSurfaceFlingerStub::insertFrameTime(Layer* layer, nsecs_t time) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->insertFrameTime(layer, time);
}

void MiSurfaceFlingerStub::insertFrameFenceTime(Layer* layer, const std::shared_ptr<FenceTime>& fence) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->insertFrameFenceTime(layer, fence);
}

bool MiSurfaceFlingerStub::isFpsStatEnabled(Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        return Istub->isFpsStatEnabled(layer);
    return false;
}

void MiSurfaceFlingerStub::enableFpsStat(Layer* layer, bool enable) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->enableFpsStat(layer, enable);
}

void MiSurfaceFlingerStub::setBadFpsStandards(Layer* layer, int targetFps, int thresh1, int thresh2) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->setBadFpsStandards(layer, targetFps, thresh1, thresh2);
}

void MiSurfaceFlingerStub::reportFpsStat(Layer* layer, Parcel* reply) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->reportFpsStat(layer, reply);
}

bool MiSurfaceFlingerStub::isFpsMonitorRunning(Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        return Istub->isFpsMonitorRunning(layer);
    return false;
}

void MiSurfaceFlingerStub::startFpsMonitor(Layer* layer, int timeout) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->startFpsMonitor(layer, timeout);
}

void MiSurfaceFlingerStub::stopFpsMonitor(Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->stopFpsMonitor(layer);
}
// END

void MiSurfaceFlingerStub::dumpDisplayFeature(std::string& result)
{
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->dumpDisplayFeature(result);
    } else {
        ALOGE("%s Istub id null", __func__);
    }
}

void MiSurfaceFlingerStub::setOrientationStatustoDF() {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setOrientationStatustoDF();
    }
}

void MiSurfaceFlingerStub::saveDefaultResolution(DisplayModes supportedModes) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->saveDefaultResolution(supportedModes);
    }
}

void MiSurfaceFlingerStub::setAffinity(const int& affinityMode) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setAffinity(affinityMode);
    }
}

void MiSurfaceFlingerStub::setreAffinity(const int& reTid) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setreAffinity(reTid);
    }
}

void MiSurfaceFlingerStub::setFrameBufferSizeforResolutionChange(sp<DisplayDevice> displayDevice) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setFrameBufferSizeforResolutionChange(displayDevice);
    }
}

#if MI_SCREEN_PROJECTION
void MiSurfaceFlingerStub::setDiffScreenProjection(uint32_t isScreenProjection, Output* outPut){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setDiffScreenProjection(isScreenProjection, outPut);
    }
}

void MiSurfaceFlingerStub::setLastFrame(uint32_t isLastFrame, Output* outPut){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setLastFrame(isLastFrame, outPut);
    }
}

void MiSurfaceFlingerStub::setCastMode(uint32_t isCastMode, Output* outPut){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setCastMode(isCastMode, outPut);
    }
}

bool MiSurfaceFlingerStub::isNeedSkipPresent(Output* outPut){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isNeedSkipPresent(outPut);
    }
    return false;
}

void MiSurfaceFlingerStub::rebuildLayerStacks(Output* outPut){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->rebuildLayerStacks(outPut);
    }
}

void MiSurfaceFlingerStub::ensureOutputLayerIfVisible(Output* outPut, const compositionengine::LayerFECompositionState* layerFEState){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->ensureOutputLayerIfVisible(outPut, layerFEState);
    }
}

bool MiSurfaceFlingerStub::includesLayer(const sp<compositionengine::LayerFE>& layerFE, const Output* outPut) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->includesLayer(layerFE, outPut);
    }
    const auto* layerFEState = layerFE->getCompositionState();
    return layerFEState && outPut->includesLayer(layerFEState->outputFilter);
}

bool MiSurfaceFlingerStub::continueComputeBounds(Layer* layer){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->continueComputeBounds(layer);
    }
    return true;
}

void MiSurfaceFlingerStub::prepareBasicGeometryCompositionState(
    compositionengine::LayerFECompositionState* compositionState, const Layer::State& drawingState, Layer* layer){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->prepareBasicGeometryCompositionState(compositionState, drawingState, layer);
    }
}

bool MiSurfaceFlingerStub::setScreenFlags(int32_t screenFlags, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setScreenFlags(screenFlags, layer);
    }
    return false;
}

void MiSurfaceFlingerStub::setChildImeFlags(uint32_t flags, uint32_t mask, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setChildImeFlags(flags, mask, layer);
    }
}

String8 MiSurfaceFlingerStub::getLayerProtoName(Layer* layer, const Layer::State& state){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->getLayerProtoName(layer, state);
    }
    return String8("");
}

void MiSurfaceFlingerStub::fillInputInfo(gui::WindowInfo* info, Layer* layer){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->fillInputInfo(info, layer);
    }
}

uint32_t MiSurfaceFlingerStub::isCastMode(){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isCastMode();
    }
    return 0;
}

void MiSurfaceFlingerStub::setIsCastMode(uint32_t castMode){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setIsCastMode(castMode);
    }
}

uint32_t MiSurfaceFlingerStub::isLastFrame(){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isLastFrame();
    }
    return 0;
}

void MiSurfaceFlingerStub::setIsLastFrame(uint32_t lastFrame){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setIsLastFrame(lastFrame);
    }
}

void MiSurfaceFlingerStub::setupNewDisplayDeviceInternal(sp<DisplayDevice>& display, const DisplayDeviceState& state){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setupNewDisplayDeviceInternal(display, state);
    }
}

void MiSurfaceFlingerStub::applyMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays, uint32_t* flags){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->applyMiuiTransactionState(miuiDisplays, flags);
    }
}

uint32_t MiSurfaceFlingerStub::setMiuiDisplayStateLocked(const MiuiDisplayState& s){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setMiuiDisplayStateLocked(s);
    }
    return 0;
}

void MiSurfaceFlingerStub::setClientStateLocked(uint64_t what, sp<Layer>& layer, uint32_t* flags,
                                                   const layer_state_t& s){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setClientStateLocked(what, layer, flags, s);
    }
}

String8 MiSurfaceFlingerStub::getDumpsysInfo(){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->getDumpsysInfo();
    }
    return String8("");
}

bool MiSurfaceFlingerStub::showContineTraverseLayersInLayerStack(const sp<Layer>& layer){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->showContineTraverseLayersInLayerStack(layer);
    }
    return false;
}

void MiSurfaceFlingerStub::setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays,
                                     uint32_t flags, int64_t desiredPresentTime){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setMiuiTransactionState(miuiDisplays, flags, desiredPresentTime);
    }
}
#endif

#if MI_VIRTUAL_DISPLAY_FRAMERATE
// SurfaceFlinger: Start
bool MiSurfaceFlingerStub::skipComposite(const sp<DisplayDevice>& display) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->skipComposite(display);
    }
    return false;
}

void MiSurfaceFlingerStub::setLimitedFrameRate(const sp<DisplayDevice>& display, const DisplayDeviceState& state){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setLimitedFrameRate(display, state);
    }
}

void MiSurfaceFlingerStub::updateLimitedFrameRate(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateLimitedFrameRate(display, currentState, drawingState);
    }
}

void MiSurfaceFlingerStub::updateDisplayStateLimitedFrameRate(uint32_t what, DisplayDeviceState& state,
            const DisplayState& s, uint32_t &flags){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateDisplayStateLimitedFrameRate(what, state, s, flags);
    }
}

void MiSurfaceFlingerStub::initLimitedFrameRate(DisplayState& d){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->initLimitedFrameRate(d);
    }
}
// End

// DisplayDevice: Start
// DisplayDevice::setLimitedFrameRate
void MiSurfaceFlingerStub::setLimitedFrameRateDisplayDevice(DisplayDevice * display, uint32_t frameRate) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setLimitedFrameRateDisplayDevice(display, frameRate);
    }
}

// DisplayDevice::needToComposite
bool MiSurfaceFlingerStub::needToCompositeDisplayDevice(DisplayDevice * display) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->needToCompositeDisplayDevice(display);
    }
    return false;
}
// End
#endif

// MIUI+ send SecureWin state
void MiSurfaceFlingerStub::setSecureProxy(sp<IBinder> secureProxy) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setSecureProxy(secureProxy);
    }
}

sp<IBinder> MiSurfaceFlingerStub::getSecureProxy() {
    sp<IBinder> ret = nullptr;
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        ret = Istub->getSecureProxy();
    }
    return ret;
}

void MiSurfaceFlingerStub::updateOutputExtra(std::shared_ptr<compositionengine::Output> output,
                bool isInternalDisplay, int sequenceId) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateOutputExtra(output, isInternalDisplay, sequenceId);
    }
}

void MiSurfaceFlingerStub::handleSecureProxy(compositionengine::Output* output,
                uint32_t layerStackId) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->handleSecureProxy(output, layerStackId);
    }
}
// END

#ifdef MI_SF_FEATURE
    void MiSurfaceFlingerStub::setNewMiSecurityDisplayState(sp<DisplayDevice>& display, const DisplayDeviceState& state) {
        IMiSurfaceFlingerStub* Istub = GetImplInstance();
        if (Istub) {
            ALOGV("MiSurfaceFlingerStub %s", __func__);
            Istub->setNewMiSecurityDisplayState(display, state);
        }
    }
    void MiSurfaceFlingerStub::setMiSecurityDisplayState(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState) {
        IMiSurfaceFlingerStub* Istub = GetImplInstance();
        if (Istub) {
            ALOGV("MiSurfaceFlingerStub %s", __func__);
            Istub->setMiSecurityDisplayState(display, currentState, drawingState);
        }
    }
    bool MiSurfaceFlingerStub::getMiSecurityDisplayState(const compositionengine::Output* output) {
        IMiSurfaceFlingerStub* Istub = GetImplInstance();
        if (Istub) {
            ALOGV("MiSurfaceFlingerStub %s", __func__);
            return Istub->getMiSecurityDisplayState(output);
        }
        return false;
    }
    void MiSurfaceFlingerStub::setVirtualDisplayCoverBlur(const Layer* layer, compositionengine::LayerFE::LayerSettings& layerSettings) {
        IMiSurfaceFlingerStub* Istub = GetImplInstance();
        if (Istub) {
            ALOGV("MiSurfaceFlingerStub %s", __func__);
            Istub->setVirtualDisplayCoverBlur(layer, layerSettings);
        }
    }
    void MiSurfaceFlingerStub::updateMiSecurityDisplayState(uint32_t what, DisplayDeviceState& state,
                const DisplayState& s, uint32_t &flags) {
        IMiSurfaceFlingerStub* Istub = GetImplInstance();
        if (Istub) {
            ALOGV("MiSurfaceFlingerStub %s", __func__);
            Istub->updateMiSecurityDisplayState(what, state, s, flags);
        }
    }
#endif

#ifdef CURTAIN_ANIM
void MiSurfaceFlingerStub::updateCurtainStatus(std::shared_ptr<compositionengine::Output> output) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateCurtainStatus(output);
    }
}

bool MiSurfaceFlingerStub::enableCurtainAnim(bool isEnable) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->enableCurtainAnim(isEnable);
    }
    return false;
}

bool MiSurfaceFlingerStub::setCurtainAnimRate(float rate) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setCurtainAnimRate(rate);
    }
    return false;
}

void MiSurfaceFlingerStub::getCurtainStatus(compositionengine::Output* output,
                                            renderengine::DisplaySettings* displaySettings) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->getCurtainStatus(output, displaySettings);
    }
}

void MiSurfaceFlingerStub::updateCurtainAnimClientComposition(bool* forceClientComposition) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateCurtainAnimClientComposition(forceClientComposition);
    }
}
#endif

#ifdef MI_SF_FEATURE
bool MiSurfaceFlingerStub::setShadowType(int shadowType, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setShadowType(shadowType, layer);
    }
    return false;
}

bool MiSurfaceFlingerStub::setShadowLength(float length, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setShadowLength(length, layer);
    }
    return false;
}

bool MiSurfaceFlingerStub::setShadowColor(const half4& color, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setShadowColor(color, layer);
    }
    return false;
}

bool MiSurfaceFlingerStub::setShadowOffset(float offsetX, float offsetY, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setShadowOffset(offsetX, offsetY, layer);
    }
    return false;
}

bool MiSurfaceFlingerStub::setShadowOutset(float outset, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setShadowOutset(outset, layer);
    }
    return false;
}

bool MiSurfaceFlingerStub::setShadowLayers(int32_t numOfLayers, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->setShadowLayers(numOfLayers, layer);
    }
    return false;
}

void MiSurfaceFlingerStub::prepareMiuiShadowClientComposition(
                                    compositionengine::LayerFE::LayerSettings& caster,
                                    const Rect& layerStackRect, Layer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->prepareMiuiShadowClientComposition(caster, layerStackRect, layer);
    }
}

void MiSurfaceFlingerStub::setShadowClientStateLocked(uint64_t what,sp<Layer>& layer,
                                    uint32_t* flags, const layer_state_t& s) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setShadowClientStateLocked(what, layer, flags, s);
    }
}
// MIUI ADD: HDR Dimmer
bool MiSurfaceFlingerStub::getHdrDimmerSupported() {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->getHdrDimmerSupported();
    }
    return false;
}
void MiSurfaceFlingerStub::updateHdrDimmerState(std::shared_ptr<compositionengine::Output> output,
                                                int colorScheme) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->updateHdrDimmerState(output, colorScheme);
    }
}
bool MiSurfaceFlingerStub::enableHdrDimmer(bool enable, float factor) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->enableHdrDimmer(enable, factor);
    }
    return false;
}
void MiSurfaceFlingerStub::getHdrDimmerState(compositionengine::Output* output,
                                            renderengine::DisplaySettings* displaySettings) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->getHdrDimmerState(output, displaySettings);
    }
}
bool MiSurfaceFlingerStub::getHdrDimmerClientComposition() {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->getHdrDimmerClientComposition();
    }
    return false;
}
// END
#endif

bool MiSurfaceFlingerStub::isFold() {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isFold();
    }
    return false;
}

bool MiSurfaceFlingerStub::isDualBuitinDisp(){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isDualBuitinDisp();
    }
    return false;
}

bool MiSurfaceFlingerStub::isCTS(){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isCTS();
    }
    return false;
}

sp<DisplayDevice> MiSurfaceFlingerStub::getActiveDisplayDevice()
{
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->getActiveDisplayDevice();
    }
    return NULL;
}

bool MiSurfaceFlingerStub::isSecondRefreshRateOverlayEnabled(){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isSecondRefreshRateOverlayEnabled();
    }
    return false;
}

bool MiSurfaceFlingerStub::isEmbeddedEnable(){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isEmbeddedEnable();
    }
    return false;
}

void MiSurfaceFlingerStub::changeLayerDataSpace(Layer* layer){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->changeLayerDataSpace(layer);
    }
}

int MiSurfaceFlingerStub::enableQsync(int qsyncMode, String8 pkgName, int pkgSetFps, int qsyncMinFps, int qsyncBaseTiming, int desiredVsync) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->enableQsync(qsyncMode, pkgName, pkgSetFps, qsyncMinFps, qsyncBaseTiming, desiredVsync);
    }
    return BAD_VALUE;
}

bool MiSurfaceFlingerStub::isQsyncEnabled()
{
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isQsyncEnabled();
    }
    return false;
}

bool MiSurfaceFlingerStub::isGlobalHBMDitherEnabled() {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->isGlobalHBMDitherEnabled();
    }
    return false;
}

// Filter unnecessary InputWindowInfo
void MiSurfaceFlingerStub::setFilterInputFlags(sp<BufferStateLayer> layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        Istub->setFilterInputFlags(layer);
    }
}

bool MiSurfaceFlingerStub::filterUnnecessaryInput(const BufferLayer* layer) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGV("MiSurfaceFlingerStub %s", __func__);
        return Istub->filterUnnecessaryInput(layer);
    }
    return true;
}

// dynamic SurfaceFlinger Log
void MiSurfaceFlingerStub::setDynamicSfLog(bool dynamic) {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->setDynamicSfLog(dynamic);
    }
}

//for dual-screen boot animation
void MiSurfaceFlingerStub::initSecondaryDisplayLocked(Vector<DisplayState>& displays){
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
            ALOGV("MiSurfaceFlingerStub %s", __func__);
            Istub->initSecondaryDisplayLocked(displays);
        }
}

bool MiSurfaceFlingerStub::isDynamicSfLog() {
    IMiSurfaceFlingerStub* Istub = GetImplInstance();
    if (Istub) {
        return Istub->isDynamicSfLog();
    }
    return false;
}
} // namespace android
