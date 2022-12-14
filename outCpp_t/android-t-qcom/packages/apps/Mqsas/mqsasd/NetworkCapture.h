/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _NETWORKCAPTURE_H_
#define _NETWORKCAPTURE_H_

#include <string>
#include <vector>
#include <linux/filter.h>

namespace android {
    class SocketFilter {
    public:
        SocketFilter(const char* srcMac, const char* dstMac, uint32_t srcIp, uint32_t dstIp,
                     uint32_t proto, uint32_t srcPort, uint32_t dstPort,
                     uint8_t *identifyData, uint32_t identifyDataLen, uint32_t retDataLen, const char* iface);

        // return: return a socket fd; if > 0, successed, otherwise, failed.
        int attach();
        // not use now
        int detach();
        // not use now
        int getSock();
    private:
        void build(const char* srcMac, const char* dstMac, uint32_t srcIp, uint32_t dstIp,
                   uint32_t proto, uint32_t srcPort, uint32_t dstPort,
                   uint8_t *identifyData, uint32_t identifyDataLen, uint32_t retDataLen);

        std::string mIface;
        int32_t mSock;
        std::vector<sock_filter> mFilterCode;
    };
}

#endif /* _NETWORKCAPTURE_H_ */
