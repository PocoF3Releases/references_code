#ifndef AIDL_GENERATED_MIUI_GREEZE_BN_MONITOR_TOKEN_H_
#define AIDL_GENERATED_MIUI_GREEZE_BN_MONITOR_TOKEN_H_

#include <binder/IInterface.h>
#include <miui/greeze/IMonitorToken.h>

namespace miui {

namespace greeze {

class BnMonitorToken : public ::android::BnInterface<IMonitorToken> {
public:
::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags = 0) override;
};  // class BnMonitorToken

}  // namespace greeze

}  // namespace miui

#endif  // AIDL_GENERATED_MIUI_GREEZE_BN_MONITOR_TOKEN_H_
