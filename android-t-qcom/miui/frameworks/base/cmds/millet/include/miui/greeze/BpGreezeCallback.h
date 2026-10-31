#ifndef AIDL_GENERATED_MIUI_GREEZE_BP_GREEZE_CALLBACK_H_
#define AIDL_GENERATED_MIUI_GREEZE_BP_GREEZE_CALLBACK_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <miui/greeze/IGreezeCallback.h>

namespace miui {

namespace greeze {

class BpGreezeCallback : public ::android::BpInterface<IGreezeCallback> {
public:
explicit BpGreezeCallback(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpGreezeCallback() = default;
::android::binder::Status reportSignal(int32_t uid, int32_t pid, int64_t now) override;
::android::binder::Status reportNet(int32_t uid, int64_t now) override;
::android::binder::Status reportBinderTrans(int32_t dstUid, int32_t dstPid, int32_t callerUid, int32_t callerPid, int32_t callerTid, bool isOneway, int64_t now, int64_t buffer) override;
::android::binder::Status reportBinderState(int32_t uid, int32_t pid, int32_t tid, int32_t binderState, int64_t now) override;
::android::binder::Status serviceReady(bool ready) override;
::android::binder::Status thawedByOther(int32_t uid, int32_t pid, int32_t module) override;
};  // class BpGreezeCallback

}  // namespace greeze

}  // namespace miui

#endif  // AIDL_GENERATED_MIUI_GREEZE_BP_GREEZE_CALLBACK_H_
