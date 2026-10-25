/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#include <utils/Log.h>
#include <net/if.h>
#include <linux/filter.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/icmp.h>
#include <linux/icmpv6.h>

#include "NetworkCapture.h"

using namespace android;
#define FAIL 0xFF

namespace {
uint32_t getMacWordFollow(uint8_t *addr) {
    return (uint32_t)(addr[0] << 24 | addr[1] << 16 | addr[2] << 8 | addr[3]);
}

uint32_t getMacHalfWordBehind(uint8_t *addr) {
    return (uint32_t)(addr[4] << 8 | addr[5]);;
}

unsigned char parseHexChar(char ch) {
    if (isdigit(ch))
        return ch - '0';
    else if ('A' <= ch && ch <= 'F')
        return ch - 'A' + 10;
    else if ('a' <= ch && ch <= 'f')
        return ch - 'a' + 10;
    else {
        ALOGE("invalid character in bssid %c", ch);
        return 0;
    }
}

void parseMacAddress(const char *mac, unsigned char *addr) {
    char *newmac = new char[strlen(mac) + 1];
    char *tmp;
    const char *d = ":";
    strcpy(newmac, mac);
    tmp = strtok(newmac, d);
    int len = 0;
    while (tmp) {
         if (strlen(tmp) != 2 || len == 6) {
             break;
         }
         addr[len++] = parseHexChar(tmp[0]) << 4 | parseHexChar(tmp[1]);
         tmp = strtok(NULL, d);
    }
    delete[] newmac;
}
}

void SocketFilter::build(const char* srcMac, const char* dstMac,
                         uint32_t srcIp, uint32_t dstIp,
                         uint32_t proto, uint32_t srcPort, uint32_t dstPort,
                         uint8_t *identifyData, uint32_t identifyDataLen, uint32_t retDataLen) {

    uint32_t ip_offset = sizeof(ether_header);
    //Modify ip_offset for mobile network request.
    if (strcmp(mIface.c_str(), "wlan0") && ((srcIp != 0 && srcIp != dstIp) ||(proto == IPPROTO_TCP))) {
        ip_offset = 0;//if current iface isn't wlan,it must be mobile iface.
    }
    // Check src mac
    if (strcmp(srcMac, "")) {
        unsigned char sMac[6] = {0, 0, 0, 0, 0, 0};
        parseMacAddress(srcMac, sMac);
        uint32_t ether_shost_offset = offsetof(ether_header, ether_shost);
        mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS,  ether_shost_offset));
        mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, getMacWordFollow(sMac), 0, FAIL));
        mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_H   | BPF_ABS,  ether_shost_offset + 4));
        mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, getMacHalfWordBehind(sMac), 0, FAIL));
    }


    // Check dst mac
    if (strcmp(dstMac, "")) {
        unsigned char dMac[6] = {0, 0, 0, 0, 0, 0};
        parseMacAddress(dstMac, dMac);
        uint32_t ether_dhost_offset = offsetof(ether_header, ether_dhost);
        mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS,  ether_dhost_offset));
        mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, getMacWordFollow(dMac), 0, FAIL));
        mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_H   | BPF_ABS,  ether_dhost_offset + 4));
        mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, getMacHalfWordBehind(dMac), 0, FAIL));
    }


    // Check src Ip
    if (srcIp) {
        uint32_t saddr_offset = ip_offset + offsetof(iphdr, saddr);
        mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS,  saddr_offset));
        mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, srcIp, 0, FAIL));
    }

    // Check dst Ip
    if (dstIp) {
        uint32_t daddr_offset = ip_offset + offsetof(iphdr, daddr);
        mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS,  daddr_offset));
        mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,dstIp, 0, FAIL));
    }

    // Check the protocol
    if (proto) {
        uint32_t proto_offset = ip_offset + offsetof(iphdr, protocol);
        mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_B   | BPF_ABS,  proto_offset));
        mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,  proto, 0, FAIL));
    }

    // Check this is not a fragment
    uint32_t flags_offset = ip_offset + offsetof(iphdr, frag_off);
    mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_H    | BPF_ABS, flags_offset));
    mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K,   0x1fff, FAIL, 0));

    uint32_t data_offset = 0;
    if (proto == IPPROTO_ICMP) {
        data_offset = ip_offset + sizeof(icmphdr);
        mFilterCode.push_back(BPF_STMT(BPF_LDX  | BPF_B    | BPF_MSH, ip_offset));
    } else {
        // Check the src port.
        if (srcPort) {
            uint32_t sport_offset = ip_offset + offsetof(tcphdr, source);
            mFilterCode.push_back(BPF_STMT(BPF_LDX | BPF_B    | BPF_MSH, ip_offset));
            mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_H    | BPF_IND, sport_offset));
            mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ  | BPF_K,   srcPort, 0, 1));
        }

        // Check the destination port.
        if (dstPort) {
            uint32_t dport_offset = ip_offset + offsetof(tcphdr, dest);
            mFilterCode.push_back(BPF_STMT(BPF_LDX | BPF_B    | BPF_MSH, ip_offset));
            mFilterCode.push_back(BPF_STMT(BPF_LD  | BPF_H    | BPF_IND, dport_offset));
            mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ  | BPF_K,   dstPort, 0, 1));
        }

        uint32_t thlen_offset = ip_offset + offsetof(tcphdr, ack_seq) + 4;
        mFilterCode.push_back(BPF_STMT(BPF_LDX  | BPF_B    | BPF_MSH, ip_offset));
        mFilterCode.push_back(BPF_STMT(BPF_LD   | BPF_B    | BPF_IND, thlen_offset));
        mFilterCode.push_back(BPF_STMT(BPF_ALU  | BPF_RSH | BPF_K, 4));
        mFilterCode.push_back(BPF_STMT(BPF_ALU  | BPF_LSH | BPF_K, 2));
        mFilterCode.push_back(BPF_STMT(BPF_ALU  | BPF_ADD | BPF_X, 0));
        mFilterCode.push_back(BPF_STMT(BPF_MISC  | BPF_TAX, 0));
        data_offset = ip_offset;
    }

    retDataLen += data_offset;

    // Check the data
    if (identifyDataLen && identifyData != NULL) {
        uint32_t wordLen = (identifyDataLen >> 2) << 2;
        for (uint32_t i = 0; i < wordLen; i+=4) {
            uint32_t dataValue = identifyData[i] << 24 | identifyData[i+1] << 16 | identifyData[i+2] << 8 | identifyData[i+3];
            mFilterCode.push_back(BPF_STMT(BPF_LD | BPF_W | BPF_IND, data_offset));
            mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ  | BPF_K, dataValue , 0, FAIL));
            data_offset += 4;
        }
        for (uint32_t i = 0; i < identifyDataLen - wordLen; i++) {
            uint32_t dataValue = identifyData[i];
            mFilterCode.push_back(BPF_STMT(BPF_LD | BPF_B | BPF_IND, data_offset++));
            mFilterCode.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ  | BPF_K, dataValue , 0, FAIL));
        }
    }

    // Accept or reject
    mFilterCode.push_back(BPF_STMT(BPF_MISC | BPF_TXA, 0));
    mFilterCode.push_back(BPF_STMT(BPF_ALU | BPF_ADD | BPF_K, retDataLen));
    mFilterCode.push_back(BPF_STMT(BPF_RET | BPF_A, 0xffff));
    mFilterCode.push_back(BPF_STMT(BPF_RET | BPF_K, 0));

    /* rewrite all FAIL jump offsets */
    size_t filter_len = mFilterCode.size();
    for (size_t idx = 0; idx < filter_len; idx++) {
        if (BPF_CLASS(mFilterCode[idx].code) == BPF_JMP) {
            if (mFilterCode[idx].jt == FAIL) {
                 mFilterCode[idx].jt = filter_len - idx - 2;
            }

            if (mFilterCode[idx].jf == FAIL) {
                mFilterCode[idx].jf = filter_len - idx - 2;
            }
        }
    }
}

SocketFilter::SocketFilter(const char* srcMac, const char* dstMac,
                               uint32_t srcIp, uint32_t dstIp,
                               uint32_t proto, uint32_t srcPort, uint32_t dstPort,
                               uint8_t *identifyData, uint32_t identifyDataLen, uint32_t retDataLen, const char* iface) :
                               mIface(iface), mSock(-1) {
    build(srcMac, dstMac, srcIp, dstIp, proto, srcPort, dstPort, identifyData, identifyDataLen, retDataLen);
}

int SocketFilter::attach() {
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (sock < 0) {
        ALOGE("SockFilter create sock failed");
        return -1;
    }

    struct sockaddr_ll ll;
    memset(&ll, 0, sizeof(ll));
    ll.sll_family = AF_PACKET;
    ll.sll_ifindex = if_nametoindex(mIface.c_str());
    ll.sll_protocol = htons(ETH_P_IP);
    if (bind(sock, (struct sockaddr *) &ll, sizeof(ll)) < 0) {
        ALOGE("attachSockFilter bind failed");
        close(sock);
        return -1;
    }

    struct sock_fprog filter = {
        (uint16_t)mFilterCode.size(),
        mFilterCode.data(),
    };

    if (setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)) != 0) {
        ALOGE("SockFilter setsockopt failed");
        close(sock);
        return -1;
    }

    mSock = sock;
    return sock;
}

int SocketFilter::detach() {
    return -1;
}

int SocketFilter::getSock() {
    return mSock;
}
