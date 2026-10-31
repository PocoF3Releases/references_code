#include "miui/greeze/IGreezeManager.h"
#include <millet.h>
#include <milletApi.h>
#include <binder/Parcel.h>
#include <binder/IServiceManager.h>
#include <miui/greeze/BnMonitorToken.h>
#include <android-base/logging.h>

//using namespace android;
#define RETRY_COUNT        40
#define RETRY_INTERVAL        5

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

static int32_t TYPE_MONITOR_BINDER = 1;
static int32_t TYPE_MONITOR_SIGNAL = 2;
static int32_t TYPE_MONITOR_NET = 3;

static inline int64_t getTime(const struct millet_data msg){
    unsigned long long sec = msg.tm.sec;
    long nsec = msg.tm.nsec;
    int64_t time = sec * 1000 + nsec/1000000;
    return time;
}

static inline void reportSignal(const struct millet_data *msg) {
    const android::sp<android::IServiceManager> sm(android::defaultServiceManager());
    if (sm == NULL)
        return;
    android::sp<android::IBinder> binder = sm->getService(android::String16("greezer"));
    if (binder == NULL)
        return;
    android::sp<miui::greeze::IGreezeManager> greezer = android::interface_cast<miui::greeze::IGreezeManager>(binder);
    int64_t time = getTime(*msg);
    greezer->reportSignal(msg->uid, msg->mod.k_priv.sig.killed_pid, time);
}

static inline void reportNet(const struct millet_data *msg) {
    const android::sp<android::IServiceManager> sm(android::defaultServiceManager());
    if (sm == NULL)
        return;
    android::sp<android::IBinder> binder = sm->getService(android::String16("greezer"));
    if (binder == NULL)
        return;
    android::sp<miui::greeze::IGreezeManager> greezer = android::interface_cast<miui::greeze::IGreezeManager>(binder);
    int64_t time = getTime(*msg);
    greezer->reportNet(msg->uid, time);
}

static inline void reportBinderTrans(const struct millet_data *msg) {
    const android::sp<android::IServiceManager> sm(android::defaultServiceManager());
    if (sm == NULL)
        return;
    android::sp<android::IBinder> binder = sm->getService(android::String16("greezer"));
    if (binder == NULL)
        return;
    android::sp<miui::greeze::IGreezeManager> greezer = android::interface_cast<miui::greeze::IGreezeManager>(binder);
    int64_t time = getTime(*msg);
    greezer->reportBinderTrans(msg->uid,
                               msg->mod.k_priv.binder.trans.dst_pid,
                               msg->mod.k_priv.binder.trans.caller_uid,
                               msg->mod.k_priv.binder.trans.caller_pid,
                               msg->mod.k_priv.binder.trans.caller_tid,
                               msg->mod.k_priv.binder.trans.tf_oneway, time, msg->pri[0]);
}

static inline void reportBinderStat(const struct millet_data *msg) {
    const android::sp<android::IServiceManager> sm(android::defaultServiceManager());
    if (sm == NULL)
        return;
    android::sp<android::IBinder> binder = sm->getService(android::String16("greezer"));
    if (binder == NULL)
        return;
    android::sp<miui::greeze::IGreezeManager> greezer = android::interface_cast<miui::greeze::IGreezeManager>(binder);
    int64_t time = getTime(*msg);
    greezer->reportBinderState(msg->uid, msg->mod.k_priv.binder.stat.pid,
                              msg->mod.k_priv.binder.stat.tid, msg->mod.k_priv.binder.stat.reason, time);
}

class MyMonitorToken : public virtual miui::greeze::BnMonitorToken {

};

static inline void reportLoopOnce(const struct millet_data *msg __unused) {
    const android::sp<android::IServiceManager> sm(android::defaultServiceManager());
    if (sm == NULL) {
        return;
    }
    android::sp<android::IBinder> binder = sm->getService(android::String16("greezer"));
    if (binder == NULL) {
        return;
    }
    android::sp<miui::greeze::IGreezeManager> greezer = android::interface_cast<miui::greeze::IGreezeManager>(binder);
    android::sp<MyMonitorToken> token = new MyMonitorToken();
    bool result = true;
    int cycle = 0;
    while (greezer == NULL) {
        LOG(INFO) << "retry :" << cycle;
        if (++cycle > RETRY_COUNT) {
            break;
        }
        sleep(RETRY_INTERVAL);
        binder = sm->getService(android::String16("greezer"));
        greezer = android::interface_cast<miui::greeze::IGreezeManager>(binder);
    }
    if(greezer == NULL) {
        LOG(INFO) << "register callback greezer == null ";
    }else {
        greezer->registerMonitor(token,TYPE_MONITOR_BINDER,&result);
        greezer->registerMonitor(token,TYPE_MONITOR_SIGNAL,&result);
        greezer->registerMonitor(token,TYPE_MONITOR_NET,&result);
    }
    greezer->reportLoopOnce();
}

void register_hook_perf_millet() {
    register_mod_hook(reportSignal, SIG_TYPE, PERF_POLICY);
    register_mod_hook(reportBinderTrans, BINDER_TYPE, PERF_POLICY);
    register_mod_hook(reportBinderStat, BINDER_ST_TYPE, PERF_POLICY);
    register_mod_hook(reportNet, PKG_TYPE, PERF_POLICY);
    register_mod_hook(reportLoopOnce, HANDSHK_TYPE, PERF_POLICY);
}