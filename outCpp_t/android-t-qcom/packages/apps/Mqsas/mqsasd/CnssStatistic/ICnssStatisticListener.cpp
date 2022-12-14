#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

#include <ICnssStatisticListener.h>
#include <binder/Parcel.h>
#include <android-base/macros.h>

namespace android
{

    IMPLEMENT_META_INTERFACE(CnssStatisticListener, "miui.mqsas.ICnssStatisticListener")

    BpCnssStatisticListener::BpCnssStatisticListener(const ::android::sp<::android::IBinder> &_aidl_impl)
        : BpInterface<ICnssStatisticListener>(_aidl_impl)
    {
    }

    ::android::binder::Status BpCnssStatisticListener::onWakeupEvent(int32_t uid, int32_t proto_subtype, int32_t src_port, int32_t dst_port)
    {
        ::android::Parcel _aidl_data;
        ::android::Parcel _aidl_reply;
        ::android::status_t _aidl_ret_status = ::android::OK;
        ::android::binder::Status _aidl_status;
        _aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
        if (((_aidl_ret_status) != (::android::OK)))
        {
            goto _aidl_error;
        }
        _aidl_ret_status = _aidl_data.writeInt32(uid);
        if (((_aidl_ret_status) != (::android::OK)))
        {
            goto _aidl_error;
        }
        _aidl_ret_status = _aidl_data.writeInt32(src_port);
        if (((_aidl_ret_status) != (::android::OK)))
        {
            goto _aidl_error;
        }
        _aidl_ret_status = _aidl_data.writeInt32(dst_port);
        if (((_aidl_ret_status) != (::android::OK)))
        {
            goto _aidl_error;
        }
        _aidl_ret_status = _aidl_data.writeInt32(proto_subtype);
        if (((_aidl_ret_status) != (::android::OK)))
        {
            goto _aidl_error;
        }

        _aidl_ret_status = remote()->transact(::android::IBinder::FIRST_CALL_TRANSACTION + 0 , _aidl_data, &_aidl_reply);
        if (((_aidl_ret_status) != (::android::OK)))
        {
            goto _aidl_error;
        }
        _aidl_ret_status = _aidl_status.readFromParcel(_aidl_reply);
        if (((_aidl_ret_status) != (::android::OK)))
        {
            goto _aidl_error;
        }
        if (!_aidl_status.isOk())
        {
            return _aidl_status;
        }
    _aidl_error:
        _aidl_status.setFromStatusT(_aidl_ret_status);
        return _aidl_status;
    }

} // namespace android

