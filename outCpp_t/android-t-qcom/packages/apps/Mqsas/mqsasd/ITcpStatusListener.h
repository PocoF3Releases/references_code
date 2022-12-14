#ifndef AIDL_GENERATED_MIUI_MQSAS_I_TCP_STATUS_LISTENER_H_
#define AIDL_GENERATED_MIUI_MQSAS_I_TCP_STATUS_LISTENER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <cstdint>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

namespace android {

class ITcpStatusListener : public ::android::IInterface {
public:
  DECLARE_META_INTERFACE(TcpStatusListener)
  virtual ::android::binder::Status onRttInfoEvent(int32_t uid, int32_t rtt, int32_t netid) = 0;
};  // class ITcpStatusListener

class BpTcpStatusListener : public ::android::BpInterface<ITcpStatusListener> {
public:
  explicit BpTcpStatusListener(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpTcpStatusListener() = default;
  ::android::binder::Status onRttInfoEvent(int32_t uid, int32_t rtt, int32_t netid) override;
};  // class BpTcpStatusListener

}  // namespace android

#endif  // AIDL_GENERATED_MIUI_MQSAS_I_TCP_STATUS_LISTENER_H_
