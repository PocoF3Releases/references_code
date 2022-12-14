#ifndef AIDL_GENERATED_MIUI_GREEZE_BP_MONITOR_TOKEN_H_
#define AIDL_GENERATED_MIUI_GREEZE_BP_MONITOR_TOKEN_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <miui/greeze/IMonitorToken.h>

namespace miui {

namespace greeze {

class BpMonitorToken : public ::android::BpInterface<IMonitorToken> {
public:
explicit BpMonitorToken(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpMonitorToken() = default;
};  // class BpMonitorToken

}  // namespace greeze

}  // namespace miui

#endif  // AIDL_GENERATED_MIUI_GREEZE_BP_MONITOR_TOKEN_H_
