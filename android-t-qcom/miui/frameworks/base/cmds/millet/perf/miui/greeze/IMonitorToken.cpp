#include <miui/greeze/IMonitorToken.h>
#include <miui/greeze/BpMonitorToken.h>

namespace miui {

namespace greeze {

IMPLEMENT_META_INTERFACE(MonitorToken, "miui.greeze.IMonitorToken")

}  // namespace greeze

}  // namespace miui
#include <miui/greeze/BpMonitorToken.h>
#include <binder/Parcel.h>

namespace miui {

namespace greeze {

BpMonitorToken::BpMonitorToken(const ::android::sp<::android::IBinder>& _aidl_impl)
    : BpInterface<IMonitorToken>(_aidl_impl){
}

}  // namespace greeze

}  // namespace miui
#include <miui/greeze/BnMonitorToken.h>
#include <binder/Parcel.h>

namespace miui {

namespace greeze {

::android::status_t BnMonitorToken::onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) {
::android::status_t _aidl_ret_status = ::android::OK;
switch (_aidl_code) {
default:
{
_aidl_ret_status = ::android::BBinder::onTransact(_aidl_code, _aidl_data, _aidl_reply, _aidl_flags);
}
break;
}
if (_aidl_ret_status == ::android::UNEXPECTED_NULL) {
_aidl_ret_status = ::android::binder::Status::fromExceptionCode(::android::binder::Status::EX_NULL_POINTER).writeToParcel(_aidl_reply);
}
return _aidl_ret_status;
}

}  // namespace greeze

}  // namespace miui
