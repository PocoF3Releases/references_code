#ifndef AIDL_GENERATED_MIUI_GREEZE_BN_GREEZE_MANAGER_H_
#define AIDL_GENERATED_MIUI_GREEZE_BN_GREEZE_MANAGER_H_

#include <binder/IInterface.h>
#include <miui/greeze/IGreezeManager.h>

namespace miui {

namespace greeze {

class BnGreezeManager : public ::android::BnInterface<IGreezeManager> {
public:
::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags = 0) override;
};  // class BnGreezeManager

}  // namespace greeze

}  // namespace miui

#endif  // AIDL_GENERATED_MIUI_GREEZE_BN_GREEZE_MANAGER_H_
