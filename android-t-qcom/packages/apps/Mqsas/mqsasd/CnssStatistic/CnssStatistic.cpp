#define LOG_TAG "mqsasd-CnssStatistic"
#include "CnssStatistic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netinet/in.h>
#include <net/if.h>

#include <pthread.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <inttypes.h>

#include <cutils/log.h>
#include <utils/Log.h>
#include "cnss_statistic_genl_msg.h"
#include "cnss_statistic_genl.h"
#include "CnssStatistic.h"

#include <netlink/genl/genl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/genl/ctrl.h>
#include <cutils/log.h>

#include <sys/wait.h>

namespace android{

#define EPOLL_EVENT_SIZE 10

static pthread_t pthread;
static std::mutex mLock;

CnssStatistic::CnssStatisticListenerMap CnssStatistic::mCnssStatisticListenerMap = CnssStatisticListenerMap();

void CnssStatistic::onWakeup(int uid, int proto_subtype, int src_port, int dst_port) {
    mLock.lock();
    for (auto& listener : mCnssStatisticListenerMap) {
        listener.first->onWakeupEvent(uid, proto_subtype, src_port, dst_port);
    }
    mLock.unlock();
}

void CnssStatistic::initMonitor()
{
    int err = pthread_create(&pthread, NULL, nl_loop_run, NULL);
    if (err != 0)
    {
        ALOGE("monitor thread create failed!!");
        return;
    }
}

void *CnssStatistic::nl_loop_run(void *args __attribute__((unused)))
{
    if (cnss_statistic_genl_init())
    {
        ALOGE("Failed to init cnss_statictic genl between daemon and driver");
        return NULL;
    }
    struct pollfd pfd[1];
    memset(&pfd, 0, sizeof(pfd[0]));
    pfd[0].fd = cnss_statistic_genl_get_fd();
    pfd[0].events = POLLIN;

    while (true)
    {
        int result = 0;
        pfd[0].revents = 0;
        result = poll(pfd, 1, -1);
        if (result < 0)
        {
            continue;
        }
        if (pfd[0].revents & (POLLIN | POLLHUP | POLLERR))
        {
            cnss_statistic_genl_recvmsgs();
        }
    }
    return NULL;
}

void CnssStatistic::registerCnssStatusListener(sp<ICnssStatisticListener>& listener) {
    ALOGD("registerCnssStatusListener: %p", &listener);
    class CnssStatus : public android::IBinder::DeathRecipient {
    public:
        CnssStatus(android::sp<ICnssStatisticListener> listener)
            : mListener(std::move(listener)) {}
            ~CnssStatus() override = default;
        void binderDied(const android::wp<android::IBinder>& /* who */) override {
            unregisterCnssStatusListener(mListener);
        }
    private:
        android::sp<ICnssStatisticListener> mListener;
    };

    android::sp<android::IBinder::DeathRecipient> cnss_status = new CnssStatus(listener);
    android::IInterface::asBinder(listener) -> linkToDeath(cnss_status);
    mLock.lock();
    mCnssStatisticListenerMap.insert({listener, cnss_status});
    mLock.unlock();
    cnss_statistic_send_enable_command();
    return;
 }

 void CnssStatistic::unregisterCnssStatusListener(sp<ICnssStatisticListener>& listener){
    ALOGD("unregisterCnssStatusListener: %p", &listener);
    cnss_statistic_send_disable_command();
    mLock.lock();
    mCnssStatisticListenerMap.erase(listener);
    mLock.unlock();
    return;
}
};

