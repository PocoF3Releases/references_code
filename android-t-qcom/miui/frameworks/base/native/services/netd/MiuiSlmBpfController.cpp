#define LOG_NDEBUG 0
#define LOG_TAG "miuiSlmBpf"

#include "MiuiSlmBpfController.h"

#include "bpf/BpfMap.h"
#include "bpf_shared.h"
#include <cutils/log.h>

#define SLM_BPF_PATH "/sys/fs/bpf"
#define SLM_BPF_UID_MONITOR_MAP SLM_BPF_PATH "/map_voipstat_uid_monitor_map"
#define SLM_BPF_VOIP_STAT_PATH SLM_BPF_PATH "/map_voipstat_voip_record_map"

#define OPT_GET_ADDR 0x1
#define OPT_GET_PORT 0x2

#define UID_ENABLE 1

typedef struct {
    uint32_t remote_port;      /* Stored in network byte order */
    union {
      uint32_t remote_ip4;       /* Stored in network byte order */
      uint32_t remote_ip6[4];    /* Stored in network byte order */
    } ip_addr;
    uint32_t uid;
    uint64_t timestamp;
} rtt_record_t;

MiuiSlmBpfController *MiuiSlmBpfController::sInstance = NULL;

unsigned int MiuiSlmBpfController::getMiuiSlmVoipUdpAddressAndPort(int uid, int opt) {
    // int ret = 0;
    android::bpf::BpfMap<uint32_t, rtt_record_t> slmBpfTcpStateRttRecordMap;

    slmBpfTcpStateRttRecordMap.reset(
        android::bpf::mapRetrieveRW(SLM_BPF_VOIP_STAT_PATH));

    ALOGD("getMiuiSlmVoipUdpAddress UID IS %d",uid);

    if (!slmBpfTcpStateRttRecordMap.isValid()) {
        // ret = -errno;
        ALOGE("open slmBpfTcpStateRttRecordMap map failed: %s", strerror(errno));
        return 0;
    }

    auto record = slmBpfTcpStateRttRecordMap.readValue(uid);
    if (!record.ok()) {
        // ret = -errno;
        ALOGE("read slmBpfTcpStateRttRecordMap map failed: %s", strerror(errno));
        return 0;
    }

    if (opt == OPT_GET_ADDR) {
        return record.value().ip_addr.remote_ip4;
    } else if (opt == OPT_GET_PORT) {
        return record.value().remote_port;
    }

    ALOGE("getMiuiSlmVoipUdpAddressAndPort opt value is error！");
    return 0;
}

unsigned int MiuiSlmBpfController::getMiuiSlmVoipUdpPort(int uid) {
    return getMiuiSlmVoipUdpAddressAndPort(uid, OPT_GET_PORT);
}

unsigned int MiuiSlmBpfController::getMiuiSlmVoipUdpAddress(int uid) {
    return getMiuiSlmVoipUdpAddressAndPort(uid, OPT_GET_ADDR);
}

bool MiuiSlmBpfController::setMiuiSlmBpfUid(int uid) {
    if (writeMiuiSlmBpfUid(uid) != 0) {
        ALOGE("set slmBpfUidMonitorMap map is error!");
        return false;
    }
    return true;
}

int MiuiSlmBpfController::writeMiuiSlmBpfUid(int uid) {
    int ret = 0;
    android::bpf::BpfMap<uint32_t, uint8_t> slmBpfUidMonitorMap;

    slmBpfUidMonitorMap.reset(
        android::bpf::mapRetrieveRW(SLM_BPF_UID_MONITOR_MAP));

    if (!slmBpfUidMonitorMap.isValid()) {
        ret = -errno;
        ALOGE("read slmBpfUidMonitorMap map failed: %s", strerror(errno));
        return ret;
    }

    if (!slmBpfUidMonitorMap.writeValue(uid, UID_ENABLE, BPF_ANY).ok()) {
        ret = -errno;
        ALOGE("write slmBpfUidMonitorMap map failed: %s", strerror(errno));
        return ret;
    }
    return ret;
}

MiuiSlmBpfController *MiuiSlmBpfController::get() {
    if (!sInstance) {
        sInstance = new MiuiSlmBpfController();
    }

    return sInstance;
}

MiuiSlmBpfController::MiuiSlmBpfController() {

}
