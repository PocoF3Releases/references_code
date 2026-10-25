#ifndef ANDROID_VOLD_VOLDSTUB_H
#define ANDROID_VOLD_VOLDSTUB_H
#include "IVoldStub.h"
#include <mutex>

#define LIBPATH "/system_ext/lib64/libvoldimpl.so"
using namespace android::vold;
using namespace android;
class VoldStub {
private:
	VoldStub() {}
	static void *LibVoldImpl;
	static IVoldStub *ImplInstance;
	static IVoldStub *GetImplInstance();
	static void DestroyImplInstance();
	static std::mutex StubLock;

public:
	virtual ~VoldStub() {}
	static int runExtMFlush(int pageCount, int flushLevel, const android::sp<android::os::IVoldTaskListener>& listener);
	static int stopExtMFlush(const android::sp<android::os::IVoldTaskListener>& listener);
	static void gcBoosterControl(const std::string& enable);
	static bool ShouldAbort();
	static void RecordTrimStart();
	static void RecordTrimFinish();
	static void StopTrimThread();
	static void SetCldListener(const android::sp<android::os::IVoldTaskListener>& listener);
	static int GetCldFragLevel();
	static int RunCldOnHal();
	static void RunUrgentGcIfNeeded(const std::list<std::string>& paths);

	static void MoveStorage(const std::string& from, const std::string& to,
                     const android::sp<android::os::IVoldTaskListener>& listener, const char *wake_lock, int flag);
	static void fixupAppDirRecursiveNative(const std::string& path, int32_t appUid, int32_t gid, int32_t flag);
	static void moveStorageQuickly(const std::string& from, const std::string& to, const android::sp<android::os::IVoldTaskListener>& listener,
                int32_t flag, int32_t* _aidl_return);
	static int fixupAppDirRecursive(const std::string& path, int32_t appUid, int32_t gid, int32_t flag);
};

#endif
