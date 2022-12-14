//
// Created by mi on 20-11-23.
//

#ifndef MQSAS_CNSSSTATISTIC_H
#define MQSAS_CNSSSTATISTIC_H
#include <vector>
#include <mutex>
#include <map>
#include "ICnssStatisticListener.h"

namespace android
{
    class CnssStatistic
    {
    public:
        static void initMonitor();
        using CnssStatisticListenerMap = std::map<sp<ICnssStatisticListener>, sp<IBinder::DeathRecipient>>;
        static void onWakeup(int uid, int proto_subtype, int src_port, int dst_port);
        static void registerCnssStatusListener(sp<ICnssStatisticListener>& listener);
        static void unregisterCnssStatusListener(sp<ICnssStatisticListener>& listener);

    private:
        static void *nl_loop_run(void *args);
        static CnssStatisticListenerMap mCnssStatisticListenerMap;
    };

};
#endif //MQSAS_CNSSSTATISTIC_H

