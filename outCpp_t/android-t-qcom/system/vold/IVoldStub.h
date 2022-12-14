#ifndef ANDROID_VOLD_IVOLDSTUB_H
#define ANDROID_VOLD_IVOLDSTUB_H

#include "android/os/IVoldTaskListener.h"
#include "VoldNativeService.h"
#include <list>
#include "model/VolumeBase.h"
#include "MoveStorage.h"
#include "VolumeManager.h"

#define VOLD_IMPL_ERROR (-8)
#define HW_NOT_SUPPORT (-9)
using namespace android::vold;
using namespace android;

class IVoldStub {
public:
	virtual ~IVoldStub() {}
	virtual int runExtMFlush(int pageCount, int flushLevel, const android::sp<android::os::IVoldTaskListener>& listener);
	virtual int stopExtMFlush(const android::sp<android::os::IVoldTaskListener>& listener);
	virtual void gcBoosterControl(const std::string& enable);
	virtual bool ShouldAbort();
	virtual void RecordTrimStart();
	virtual void RecordTrimFinish();
	virtual void StopTrimThread();
	virtual void SetCldListener(const android::sp<android::os::IVoldTaskListener>& listener);
	virtual int GetCldFragLevel();
	virtual int RunCldOnHal();
	virtual void RunUrgentGcIfNeeded(const std::list<std::string>& paths);
	virtual void MoveStorage(const std::string& from, const std::string& to,
                const android::sp<android::os::IVoldTaskListener>& listener, const char *wake_lock, int flag);
	virtual void fixupAppDirRecursiveNative(const std::string& path, int32_t appUid, int32_t gid, int32_t flag);
	virtual void moveStorageQuickly(const std::string& from, const std::string& to, const android::sp<android::os::IVoldTaskListener>& listener,
                int32_t flag, int32_t* _aidl_return);
	virtual int fixupAppDirRecursive(const std::string& path, int32_t appUid, int32_t gid, int32_t flag);
};

typedef IVoldStub* Create();
typedef void Destroy(IVoldStub *);

#endif
