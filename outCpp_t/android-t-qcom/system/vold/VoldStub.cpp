#include <iostream>
#include <dlfcn.h>
#include <android-base/logging.h>
#include "VoldStub.h"


void* VoldStub::LibVoldImpl = NULL;
IVoldStub* VoldStub::ImplInstance = NULL;
std::mutex VoldStub::StubLock;

IVoldStub* VoldStub::GetImplInstance() {
	std::lock_guard<std::mutex> lock(StubLock);
	if (!ImplInstance) {
		LibVoldImpl = dlopen(LIBPATH, RTLD_LAZY);
		if (LibVoldImpl) {
			Create* create = (Create *)dlsym(LibVoldImpl, "create");
			ImplInstance = create();
		}else{
		ALOGE("%s",dlerror());
        }

	}
	return ImplInstance;
}

void VoldStub::DestroyImplInstance() {
	std::lock_guard<std::mutex> lock(StubLock);
	if (LibVoldImpl) {
		Destroy* destroy = (Destroy *)dlsym(LibVoldImpl, "destroy");
		destroy(ImplInstance);
		dlclose(LibVoldImpl);
		LibVoldImpl = NULL;
		ImplInstance = NULL;
	}
}

int VoldStub::runExtMFlush(int pageCount, int flushLevel, const android::sp<android::os::IVoldTaskListener>& listener) {
	int ret = VOLD_IMPL_ERROR;
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		ret = Istub->runExtMFlush(pageCount, flushLevel, listener);
	return ret;
}

int VoldStub::stopExtMFlush(const android::sp<android::os::IVoldTaskListener>& listener) {
	int ret = VOLD_IMPL_ERROR;
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		ret = Istub->stopExtMFlush(listener);
	return ret;
}

void VoldStub::gcBoosterControl(const std::string& enable) {
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
        Istub->gcBoosterControl(enable);
}

bool VoldStub::ShouldAbort() {
	bool ret = false;
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		ret = Istub->ShouldAbort();
	return ret;
}

void VoldStub::RecordTrimStart() {
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		Istub->RecordTrimStart();
}

void VoldStub::RecordTrimFinish() {
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		Istub->RecordTrimFinish();
}

void VoldStub::StopTrimThread() {
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		Istub->StopTrimThread();
}


void VoldStub::SetCldListener(const android::sp<android::os::IVoldTaskListener>& listener) {
	bool ret = false;
	IVoldStub* Istub = GetImplInstance();
	if (Istub)
		Istub->SetCldListener(listener);
}

int VoldStub::GetCldFragLevel() {
	int ret = -1;
	IVoldStub* Istub = GetImplInstance();
	if (Istub)
		ret = Istub->GetCldFragLevel();
	return ret;
}

int VoldStub::RunCldOnHal() {
	int ret = -1;
	IVoldStub* Istub = GetImplInstance();
	if (Istub)
		ret = Istub->RunCldOnHal();
	return ret;
}

void VoldStub::RunUrgentGcIfNeeded(const std::list<std::string>& paths) {
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		Istub->RunUrgentGcIfNeeded(paths);
}

void VoldStub::MoveStorage(const std::string& from, const std::string& to,
                 const android::sp<android::os::IVoldTaskListener>& listener, const char *wake_lock, int flag) {
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		Istub->MoveStorage(from, to, listener, wake_lock, flag);
}

void VoldStub::fixupAppDirRecursiveNative(const std::string& path, int32_t appUid, int32_t gid, int32_t flag) {
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		Istub->fixupAppDirRecursiveNative(path, appUid, gid, flag);
}
void VoldStub::moveStorageQuickly(const std::string& from, const std::string& to, const android::sp<android::os::IVoldTaskListener>& listener,
                int32_t flag, int32_t* _aidl_return) {
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		Istub->moveStorageQuickly(from, to, listener, flag, _aidl_return);
}

int VoldStub::fixupAppDirRecursive(const std::string& path, int32_t appUid, int32_t gid, int32_t flag) {
	int ret = VOLD_IMPL_ERROR;
	IVoldStub* Istub = GetImplInstance();
	if(Istub)
		ret = Istub->fixupAppDirRecursive(path, appUid, gid, flag);
	return ret;
}
