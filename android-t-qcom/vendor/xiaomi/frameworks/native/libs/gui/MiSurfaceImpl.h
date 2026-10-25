#ifndef ANDROID_MI_SURFACE_IMPL_H
#define ANDROID_MI_SURFACE_IMPL_H

#include <log/log.h>
#include <utils/Errors.h>
#include <utils/String8.h>
#include <cutils/trace.h>
#include <gui/Surface.h>
#include <utils/Mutex.h>
#include "IMiSurfaceStub.h"

#include <thread>
#include <sys/inotify.h>
#include <limits.h>

//sizeof(struct inotify_event) + NAME_MAX + 1;will be sufficient to read at least one event
#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

namespace android {

class MiSurfaceImpl : public IMiSurfaceStub {
public:
    virtual ~MiSurfaceImpl();
    void skipSlotRef(sp<Surface> surface, int i) override;
    void doDynamicFps() override;
    int  doNotifyFbc(const int32_t& value)  override;
    int  DoNotifyFbc(const int32_t& value, const uint64_t& bufID, const int32_t& param, const uint64_t& id) override;
    bool supportFEAS() override;
private:
    int mDynamicFps = INT_MAX;
    bool mSupportDF = false;
    // mLastQueueBufferTimestamp means first queueBuffer TS 10s after Surface instantiated
    // after that, if support DF, then its meaning changed to last queueBuffer TS
    int64_t mLastQueueBufferTimestamp = NATIVE_WINDOW_TIMESTAMP_AUTO;
    int64_t mLastUpdateTimestamp = 0;
    mutable Mutex mMutex;
    std::string mLastConsumerPkg{"defaultPkgName"};
    int64_t fdInotify = -1;
    int64_t wdInotify = -1;
    int resReadInotify = -1;
    char bufInotify[BUF_LEN];
    bool flagInotify = false;
    //Parameters for DynamicFps
    std::string mDFProcName{""};
    int64_t mCurrentDFPid = -1;

    void updateDynamicFPSConfig(std::string mCurrentPkg);
    // get the packageName of current pid
    void getDynamicFpsPidName(int mCurrentDFPid);
};
//For FEAS
static bool mInited = false;
static bool mEnableFEAS = false;

extern "C" IMiSurfaceStub* create();
extern "C" void destroy(IMiSurfaceStub* impl);

}//namespace android

#endif
