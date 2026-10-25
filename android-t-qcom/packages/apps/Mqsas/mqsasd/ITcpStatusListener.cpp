#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

#include <ITcpStatusListener.h>
#include <binder/Parcel.h>
#include <android-base/macros.h>

namespace android {

IMPLEMENT_META_INTERFACE(TcpStatusListener, "miui.mqsas.ITcpStatusListener")

BpTcpStatusListener::BpTcpStatusListener(const ::android::sp<::android::IBinder>& _aidl_impl)
    : BpInterface<ITcpStatusListener>(_aidl_impl){
}

::android::binder::Status BpTcpStatusListener::onRttInfoEvent(int32_t uid, int32_t rtt, int32_t netid) {
  ::android::Parcel _aidl_data;
  ::android::Parcel _aidl_reply;
  ::android::status_t _aidl_ret_status = ::android::OK;
  ::android::binder::Status _aidl_status;
  _aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
  if (((_aidl_ret_status) != (::android::OK))) {
    goto _aidl_error;
  }
  _aidl_ret_status = _aidl_data.writeInt32(uid);
  if (((_aidl_ret_status) != (::android::OK))) {
    goto _aidl_error;
  }
  _aidl_ret_status = _aidl_data.writeInt32(rtt);
  if (((_aidl_ret_status) != (::android::OK))) {
    goto _aidl_error;
  }
  _aidl_ret_status = _aidl_data.writeInt32(netid);
  if (((_aidl_ret_status) != (::android::OK))) {
    goto _aidl_error;
  }

  _aidl_ret_status = remote()->transact(::android::IBinder::FIRST_CALL_TRANSACTION + 0 /* onRttInfoEvent */, _aidl_data, &_aidl_reply);
  if (((_aidl_ret_status) != (::android::OK))) {
    goto _aidl_error;
  }
  _aidl_ret_status = _aidl_status.readFromParcel(_aidl_reply);
  if (((_aidl_ret_status) != (::android::OK))) {
    goto _aidl_error;
  }
  if (!_aidl_status.isOk()) {
    return _aidl_status;
  }
  _aidl_error:
  _aidl_status.setFromStatusT(_aidl_ret_status);
  return _aidl_status;
}

}  // namespace android

