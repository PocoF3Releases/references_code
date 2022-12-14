/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _PSIMONITOR_H_
#define _PSIMONITOR_H_

#include <vector>
#include <unordered_set>
#include <mutex>

namespace android {

class PSIMonitor {
public:
    static void initPSIMonitor();
    static int getPSISocketFd();
    static void addPSITriggerCommand(int callbackKey, const char* node, const char* command);
    static int connectPSIMonitor(const char* clientKey);
private:
    static void* pthread_run(void* args);
private:
    struct PsiMonitorConnection {
        char* clientKey;
        int socketWriteFd;
        int socketReadFd;
    };
    static std::unordered_set<int> registerTriggerKeys;
    static std::vector<PsiMonitorConnection*> psiMonitorConnections;
    static std::mutex psiMonitorConnectionsLock;

};

};//namespace android

#endif /* _PSIMONITOR_H_ */
