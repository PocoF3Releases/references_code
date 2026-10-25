#ifndef AIDL_GENERATED_MIUI_GREEZE_I_GREEZE_CALLBACK_H_
#define AIDL_GENERATED_MIUI_GREEZE_I_GREEZE_CALLBACK_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <cstdint>
#include <utils/StrongPointer.h>

namespace miui {

namespace greeze {

class IGreezeCallback : public ::android::IInterface {
public:
DECLARE_META_INTERFACE(GreezeCallback)
virtual ::android::binder::Status reportSignal(int32_t uid, int32_t pid, int64_t now) = 0;
virtual ::android::binder::Status reportNet(int32_t uid, int64_t now) = 0;
virtual ::android::binder::Status reportBinderTrans(int32_t dstUid, int32_t dstPid, int32_t callerUid, int32_t callerPid, int32_t callerTid, bool isOneway, int64_t now, int64_t buffer) = 0;
virtual ::android::binder::Status reportBinderState(int32_t uid, int32_t pid, int32_t tid, int32_t binderState, int64_t now) = 0;
virtual ::android::binder::Status serviceReady(bool ready) = 0;
virtual ::android::binder::Status thawedByOther(int32_t uid, int32_t pid, int32_t module) = 0;
enum Call {
  REPORTSIGNAL = ::android::IBinder::FIRST_CALL_TRANSACTION + 0,
  REPORTNET = ::android::IBinder::FIRST_CALL_TRANSACTION + 1,
  REPORTBINDERTRANS = ::android::IBinder::FIRST_CALL_TRANSACTION + 2,
  REPORTBINDERSTATE = ::android::IBinder::FIRST_CALL_TRANSACTION + 3,
  SERVICEREADY = ::android::IBinder::FIRST_CALL_TRANSACTION + 4,
  THAWEDBYOTHER = ::android::IBinder::FIRST_CALL_TRANSACTION + 5,
};
};  // class IGreezeCallback

}  // namespace greeze

}  // namespace miui

#endif  // AIDL_GENERATED_MIUI_GREEZE_I_GREEZE_CALLBACK_H_
