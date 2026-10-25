#ifndef AIDL_GENERATED_MIUI_GREEZE_BP_GREEZE_MANAGER_H_
#define AIDL_GENERATED_MIUI_GREEZE_BP_GREEZE_MANAGER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <miui/greeze/IGreezeManager.h>

namespace miui {

namespace greeze {

class BpGreezeManager : public ::android::BpInterface<IGreezeManager> {
public:
explicit BpGreezeManager(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpGreezeManager() = default;
::android::binder::Status reportSignal(int32_t uid, int32_t pid, int64_t now) override;
::android::binder::Status reportNet(int32_t uid, int64_t now) override;
::android::binder::Status reportBinderTrans(int32_t dstUid, int32_t dstPid, int32_t callerUid, int32_t callerPid, int32_t callerTid, bool isOneway, int64_t now, int32_t buffer) override;
::android::binder::Status reportBinderState(int32_t uid, int32_t pid, int32_t tid, int32_t binderState, int64_t now) override;
::android::binder::Status freezePids(const ::std::vector<int32_t>& pids, int64_t timeout, int32_t fromWho, const ::android::String16& reason, bool* _aidl_return) override;
::android::binder::Status freezeUids(const ::std::vector<int32_t>& uids, int64_t timeout, int32_t fromWho, const ::android::String16& reason, bool checkAudioGps, bool* _aidl_return) override;
::android::binder::Status thawPids(const ::std::vector<int32_t>& pids, int32_t fromWho, const ::android::String16& reason, bool* _aidl_return) override;
::android::binder::Status thawUids(const ::std::vector<int32_t>& uids, int32_t fromWho, const ::android::String16& reason, bool* _aidl_return) override;
::android::binder::Status thawAll(int32_t module, int32_t fromWho, const ::android::String16& reason, bool* _aidl_return) override;
::android::binder::Status getFrozenPids(int32_t module, ::std::vector<int32_t>* _aidl_return) override;
::android::binder::Status isUidActive(int32_t uid, bool* _aidl_return) override;
::android::binder::Status registerMonitor(const ::android::sp<::miui::greeze::IMonitorToken>& token, int32_t type, bool* _aidl_return) override;
::android::binder::Status reportLoopOnce() override;
::android::binder::Status getFrozenUids(int32_t module, ::std::vector<int32_t>* _aidl_return) override;
::android::binder::Status registerCallback(const ::android::sp<::miui::greeze::IGreezeCallback>& callback, int32_t module, bool* _aidl_return) override;
::android::binder::Status getLastThawedTime(int32_t uid, int32_t module, int64_t* _aidl_return) override;
::android::binder::Status isUidFrozen(int32_t uid, bool* _aidl_return) override;
};  // class BpGreezeManager

}  // namespace greeze

}  // namespace miui

#endif  // AIDL_GENERATED_MIUI_GREEZE_BP_GREEZE_MANAGER_H_
