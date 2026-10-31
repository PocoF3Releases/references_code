/*
 * Copyright (C) 2022, Xiaomi Inc. All rights reserved.
 *
*/

#ifndef ANDROID_QSYNC_FEATURE_H
#define ANDROID_QSYNC_FEATURE_H

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
#include <utils/String8.h>
#include "MiSurfaceFlingerImpl.h"

#define MAX_CYCLE_COUNT 100
#define DESIRED_VSYNC_OFFSET 24
#define DESIRED_VSYNC_OFFSET_MASK 0xFF000000

#ifdef QTI_DISPLAY_CONFIG_ENABLED
#include <hardware/hwcomposer_defs.h>
#include <config/client_interface.h>
namespace DisplayConfig {
    class ClientInterface;
}
#endif

namespace android {

class SurfaceFlinger;
class MiSurfaceFlingerImpl;

class QsyncFeature : public virtual RefBase {
public:
    QsyncFeature();
    void qsyncInit(MiSurfaceFlingerImpl* misf, const sp<SurfaceFlinger>& flinger);
    bool isSupportQsync() { return mQsyncSupport; }
    bool isQsyncEnabled() { return mQsyncEnableStatus; }
    int updateQsyncStatus(int qsyncMode, String8 pkgName, int pkgSetFps = 0, int qsyncMinFps = 0, int qsyncBaseTiming = 0, int desiredVsyncHz = 0);
    int32_t getQsyncTimingModeGroup();
#ifdef QTI_DISPLAY_CONFIG_ENABLED
    ::DisplayConfig::ClientInterface *mDisplayConfigIntf = nullptr;
#endif

private:
    friend class MiSurfaceFlingerImpl;

    void ParseQsyncConfig();
    int enableQsync(int qsyncMode);
    void updateQsyncMinFps();
    void updateVsync(bool fakeVsyncEnable = false);
    void selectBestQsyncMinFps(int qsyncBaseTiming, int pkgSetFps);
    void forceClearQsyncStatus();
    int  findBaseTimingHwcId();

    sp<SurfaceFlinger> mFlinger;
    MiSurfaceFlingerImpl* mMiFlinger;

    uint32_t disp = 0;
    bool mQsyncSupport;
    int mMaxQsyncFps;
    int mMinQsyncFps;
    int mQsyncMode = 0;
    int mPkgSetFps = 0;
    String8 mPkgName;
    int mQsyncMinFps = 0;
    int mLastQsyncMinFps = 0;
    int mQsyncBaseTiming = 0;
    int mDesiredVsyncHz = 0;
    bool mQsyncEnableStatus = false;
    int mLastDesiredVsyncHz = 0;
    Mutex mQsyncStateMutex;
    static constexpr size_t kMinimumSamplesForPrediction = 6;
    bool mUseDynamicQsyncMinFps = false;
    uint32_t mQsyncBaseTimingHwcId = 1;

    /* true: use qsync timing mode
    * false: use normal timing mode*/
    bool mUseQsyncTimingMode = true;
    int32_t mQsyncTimingModeGroup = -1;
};

}
#endif
