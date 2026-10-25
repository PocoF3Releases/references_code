#define LOG_NDEBUG 0
#define LOG_TAG "qos_controller"

#include <cutils/log.h>
#include <netinet/ip.h>
#include <string>
#include <stdlib.h>
#include <sys/wait.h>

#include "NetdConstants.h"
#include "NetdConstantsExtend.h"
#include "DiffServController.h"

#define TOS_MANGLE_CHAIN_NAME    "qos_reset_OUTPUT"

DiffServController *DiffServController::sInstance = NULL;

DiffServController *DiffServController::get() {
    if (!sInstance) {
        sInstance = new DiffServController();
    }
    return sInstance;
}

DiffServController::DiffServController() : mEnabled(0) {
    char *cmds;
    asprintf(&cmds, "*mangle\n-D OUTPUT -j %s\nCOMMIT\n", TOS_MANGLE_CHAIN_NAME);
    if (execIptablesRestoreFunction(V4V6, cmds)) {
        ALOGE("iptables exec failed %d", __LINE__);
    }
    asprintf(&cmds, "*mangle\n:%s -\nCOMMIT\n", TOS_MANGLE_CHAIN_NAME);
    if (execIptablesRestoreFunction(V4V6, cmds)) {
        ALOGE("iptables exec failed %d", __LINE__);
    }
    free(cmds);
}

int DiffServController::enable() {
    if (!mEnabled) {
        char *cmds;
        asprintf(&cmds, "*mangle\n-A OUTPUT -j %s\nCOMMIT\n", TOS_MANGLE_CHAIN_NAME);
        if (execIptablesRestoreFunction(V4V6, cmds)) {
            ALOGE("iptables exec failed %d", __LINE__);
        }
        free(cmds);
        mEnabled = 1;
    }
    return 0;
}

int DiffServController::disable() {
    if (mEnabled) {
        char *cmds;
        asprintf(&cmds, "*mangle\n-D OUTPUT -j %s\nCOMMIT\n", TOS_MANGLE_CHAIN_NAME);
        if (execIptablesRestoreFunction(V4V6, cmds)) {
            ALOGE("iptables exec failed %d", __LINE__);
        }
        asprintf(&cmds, "*mangle\n:%s -\nCOMMIT\n", TOS_MANGLE_CHAIN_NAME);
        if (execIptablesRestoreFunction(V4V6, cmds)) {
            ALOGE("iptables exec failed %d", __LINE__);
        }
        free(cmds);
        mEnabled = 0;
    }
    return 0;
}

int DiffServController::setUidQos(uint32_t protocol, const char* uid, int tos, bool add) {
    int res = 0;
    char dscp[20] = {0};
    snprintf(dscp, 19, "0x%x", tos);
    // protocol field
    const char* protoStr;
    if (protocol == IPPROTO_TCP) {
        protoStr = "tcp";
    } else if (protocol == IPPROTO_UDP) {
        protoStr = "udp";
    } else {
        protoStr = "all";
    }
    // Add or Delete action
    const char* opFlag = add ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*mangle\n%s %s -p %s  -m owner --uid-owner %s -j TOS --set-tos %s\nCOMMIT\n"
            , opFlag, TOS_MANGLE_CHAIN_NAME, protoStr, uid, dscp);
    res = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (res) {
        ALOGE("iptables exec DiffServController failed %d", __LINE__);
    }
    return res;
}
