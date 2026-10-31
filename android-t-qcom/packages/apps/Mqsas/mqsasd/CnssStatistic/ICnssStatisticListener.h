#ifndef AIDL_GENERATED_MIUI_MQSAS_I_CNSS_STATISTIC_LISTENER_H_
#define AIDL_GENERATED_MIUI_MQSAS_I_CNSS_STATISTIC_LISTENER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <cstdint>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

namespace android
{
    class ICnssStatisticListener : public ::android::IInterface
    {
    public:
        DECLARE_META_INTERFACE(CnssStatisticListener)
        virtual ::android::binder::Status onWakeupEvent(int32_t uid, int32_t proto_subtype, int32_t rtt, int32_t netid) = 0;
    }; // class ICnssStatisticListener

    class BpCnssStatisticListener : public ::android::BpInterface<ICnssStatisticListener>
    {
    public:
        explicit BpCnssStatisticListener(const ::android::sp<::android::IBinder> &_aidl_impl);
        virtual ~BpCnssStatisticListener() = default;
        ::android::binder::Status onWakeupEvent(int32_t uid, int32_t proto_subtype, int32_t rtt, int32_t netid) override;
    }; // class BpCnssStatisticListener

} // namespace android

#endif // AIDL_GENERATED_MIUI_MQSAS_I_CNSS_STATISTIC_LISTENER_H_

