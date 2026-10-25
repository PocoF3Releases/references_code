#ifndef AIDL_GENERATED_MIUI_GREEZE_I_GREEZE_MANAGER_H_
#define AIDL_GENERATED_MIUI_GREEZE_I_GREEZE_MANAGER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <cstdint>
#include <miui/greeze/IGreezeCallback.h>
#include <miui/greeze/IMonitorToken.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>
#include <vector>

using namespace android;

namespace miui {

namespace greeze {

class IGreezeManager : public ::android::IInterface {
public:
DECLARE_META_INTERFACE(GreezeManager)
enum  : int32_t {
  GREEZER_MODULE_UNKNOWN = 0,
  GREEZER_MODULE_POWER = 1,
  GREEZER_MODULE_PERFORMANCE = 2,
  GREEZER_MODULE_GAME = 3,
  GREEZER_MODULE_COUNT = 4,
  GREEZER_MODULE_ALL = 9999,
  TYPE_MONITOR_BINDER = 1,
  TYPE_MONITOR_SIGNAL = 2,
  TYPE_MONITOR_NET = 3,
};
virtual ::android::binder::Status reportSignal(int32_t uid, int32_t pid, int64_t now) = 0;
virtual ::android::binder::Status reportNet(int32_t uid, int64_t now) = 0;
virtual ::android::binder::Status reportBinderTrans(int32_t dstUid, int32_t dstPid, int32_t callerUid, int32_t callerPid, int32_t callerTid, bool isOneway, int64_t now, int32_t buffer) = 0;
virtual ::android::binder::Status reportBinderState(int32_t uid, int32_t pid, int32_t tid, int32_t binderState, int64_t now) = 0;
virtual ::android::binder::Status freezePids(const ::std::vector<int32_t>& pids, int64_t timeout, int32_t fromWho, const ::android::String16& reason, bool* _aidl_return) = 0;
virtual ::android::binder::Status freezeUids(const ::std::vector<int32_t>& uids, int64_t timeout, int32_t fromWho, const ::android::String16& reason, bool checkAudioGps, bool* _aidl_return) = 0;
virtual ::android::binder::Status thawPids(const ::std::vector<int32_t>& pids, int32_t fromWho, const ::android::String16& reason, bool* _aidl_return) = 0;
virtual ::android::binder::Status thawUids(const ::std::vector<int32_t>& uids, int32_t fromWho, const ::android::String16& reason, bool* _aidl_return) = 0;
virtual ::android::binder::Status thawAll(int32_t module, int32_t fromWho, const ::android::String16& reason, bool* _aidl_return) = 0;
virtual ::android::binder::Status getFrozenPids(int32_t module, ::std::vector<int32_t>* _aidl_return) = 0;
virtual ::android::binder::Status isUidActive(int32_t uid, bool* _aidl_return) = 0;
virtual ::android::binder::Status registerMonitor(const ::android::sp<::miui::greeze::IMonitorToken>& token, int32_t type, bool* _aidl_return) = 0;
virtual ::android::binder::Status reportLoopOnce() = 0;
virtual ::android::binder::Status getFrozenUids(int32_t module, ::std::vector<int32_t>* _aidl_return) = 0;
virtual ::android::binder::Status registerCallback(const ::android::sp<::miui::greeze::IGreezeCallback>& callback, int32_t module, bool* _aidl_return) = 0;
virtual ::android::binder::Status getLastThawedTime(int32_t uid, int32_t module, int64_t* _aidl_return) = 0;
virtual ::android::binder::Status isUidFrozen(int32_t uid, bool* _aidl_return) = 0;
enum Call {
  REPORTSIGNAL = ::android::IBinder::FIRST_CALL_TRANSACTION + 0,
  REPORTNET = ::android::IBinder::FIRST_CALL_TRANSACTION + 1,
  REPORTBINDERTRANS = ::android::IBinder::FIRST_CALL_TRANSACTION + 2,
  REPORTBINDERSTATE = ::android::IBinder::FIRST_CALL_TRANSACTION + 3,
  FREEZEPIDS = ::android::IBinder::FIRST_CALL_TRANSACTION + 4,
  FREEZEUIDS = ::android::IBinder::FIRST_CALL_TRANSACTION + 5,
  THAWPIDS = ::android::IBinder::FIRST_CALL_TRANSACTION + 6,
  THAWUIDS = ::android::IBinder::FIRST_CALL_TRANSACTION + 7,
  THAWALL = ::android::IBinder::FIRST_CALL_TRANSACTION + 8,
  GETFROZENPIDS = ::android::IBinder::FIRST_CALL_TRANSACTION + 9,
  ISUIDACTIVE = ::android::IBinder::FIRST_CALL_TRANSACTION + 10,
  REGISTERMONITOR = ::android::IBinder::FIRST_CALL_TRANSACTION + 11,
  REPORTLOOPONCE = ::android::IBinder::FIRST_CALL_TRANSACTION + 12,
  GETFROZENUIDS = ::android::IBinder::FIRST_CALL_TRANSACTION + 13,
  REGISTERCALLBACK = ::android::IBinder::FIRST_CALL_TRANSACTION + 14,
  GETLASTTHAWEDTIME = ::android::IBinder::FIRST_CALL_TRANSACTION + 15,
  ISUIDFROZEN = ::android::IBinder::FIRST_CALL_TRANSACTION + 16,
};
};  // class IGreezeManager

}  // namespace greeze

}  // namespace miui

#endif  // AIDL_GENERATED_MIUI_GREEZE_I_GREEZE_MANAGER_H_
