#include <miui/greeze/IGreezeCallback.h>
#include <miui/greeze/BpGreezeCallback.h>

namespace miui {

namespace greeze {

IMPLEMENT_META_INTERFACE(GreezeCallback, "miui.greeze.IGreezeCallback")

}  // namespace greeze

}  // namespace miui
#include <miui/greeze/BpGreezeCallback.h>
#include <binder/Parcel.h>

namespace miui {

namespace greeze {

BpGreezeCallback::BpGreezeCallback(const ::android::sp<::android::IBinder>& _aidl_impl)
    : BpInterface<IGreezeCallback>(_aidl_impl){
}

::android::binder::Status BpGreezeCallback::reportSignal(int32_t uid, int32_t pid, int64_t now) {
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
_aidl_ret_status = _aidl_data.writeInt32(pid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt64(now);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IGreezeCallback::REPORTSIGNAL, _aidl_data, &_aidl_reply, ::android::IBinder::FLAG_ONEWAY);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpGreezeCallback::reportNet(int32_t uid, int64_t now) {
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
_aidl_ret_status = _aidl_data.writeInt64(now);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IGreezeCallback::REPORTNET, _aidl_data, &_aidl_reply, ::android::IBinder::FLAG_ONEWAY);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpGreezeCallback::reportBinderTrans(int32_t dstUid, int32_t dstPid, int32_t callerUid, int32_t callerPid, int32_t callerTid, bool isOneway, int64_t now, int64_t buffer) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(dstUid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(dstPid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(callerUid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(callerPid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(callerTid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeBool(isOneway);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt64(now);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt64(buffer);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IGreezeCallback::REPORTBINDERTRANS, _aidl_data, &_aidl_reply, ::android::IBinder::FLAG_ONEWAY);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpGreezeCallback::reportBinderState(int32_t uid, int32_t pid, int32_t tid, int32_t binderState, int64_t now) {
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
_aidl_ret_status = _aidl_data.writeInt32(pid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(tid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(binderState);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt64(now);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IGreezeCallback::REPORTBINDERSTATE, _aidl_data, &_aidl_reply, ::android::IBinder::FLAG_ONEWAY);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpGreezeCallback::serviceReady(bool ready) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeBool(ready);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IGreezeCallback::SERVICEREADY, _aidl_data, &_aidl_reply, ::android::IBinder::FLAG_ONEWAY);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpGreezeCallback::thawedByOther(int32_t uid, int32_t pid, int32_t module) {
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
_aidl_ret_status = _aidl_data.writeInt32(pid);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(module);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IGreezeCallback::THAWEDBYOTHER, _aidl_data, &_aidl_reply, ::android::IBinder::FLAG_ONEWAY);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

}  // namespace greeze

}  // namespace miui
#include <miui/greeze/BnGreezeCallback.h>
#include <binder/Parcel.h>

namespace miui {

namespace greeze {

::android::status_t BnGreezeCallback::onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) {
::android::status_t _aidl_ret_status = ::android::OK;
switch (_aidl_code) {
case Call::REPORTSIGNAL:
{
int32_t in_uid;
int32_t in_pid;
int64_t in_now;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_uid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_pid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt64(&in_now);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(reportSignal(in_uid, in_pid, in_now));
}
break;
case Call::REPORTNET:
{
int32_t in_uid;
int64_t in_now;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_uid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt64(&in_now);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(reportNet(in_uid, in_now));
}
break;
case Call::REPORTBINDERTRANS:
{
int32_t in_dstUid;
int32_t in_dstPid;
int32_t in_callerUid;
int32_t in_callerPid;
int32_t in_callerTid;
bool in_isOneway;
int64_t in_now;
int64_t in_buffer;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_dstUid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_dstPid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_callerUid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_callerPid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_callerTid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readBool(&in_isOneway);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt64(&in_now);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt64(&in_buffer);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(reportBinderTrans(in_dstUid, in_dstPid, in_callerUid, in_callerPid, in_callerTid, in_isOneway, in_now, in_buffer));
}
break;
case Call::REPORTBINDERSTATE:
{
int32_t in_uid;
int32_t in_pid;
int32_t in_tid;
int32_t in_binderState;
int64_t in_now;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_uid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_pid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_tid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_binderState);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt64(&in_now);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(reportBinderState(in_uid, in_pid, in_tid, in_binderState, in_now));
}
break;
case Call::SERVICEREADY:
{
bool in_ready;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readBool(&in_ready);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(serviceReady(in_ready));
}
break;
case Call::THAWEDBYOTHER:
{
int32_t in_uid;
int32_t in_pid;
int32_t in_module;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_uid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_pid);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_module);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(thawedByOther(in_uid, in_pid, in_module));
}
break;
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
