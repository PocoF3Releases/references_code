/*
 * Copyright (c) 2015-2018, 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedServiceUtils"
#include <common/AVLog.h>
#include <cutils/properties.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/ByteUtils.h>
#include <media/stagefright/rtsp/MyHandler.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include "mediaplayerservice/AVMediaServiceExtensions.h"
#include "mediaplayerservice/ExtendedServiceUtils.h"

namespace android {

AVMediaServiceUtils *createExtendedMediaServiceUtils() {
    return new ExtendedServiceUtils;
}

//static
bool ExtendedServiceUtils::pokeAHole(sp<MyHandler> /*handler*/, int rtpSocket,
        int rtcpSocket, const AString &transport, const AString &sessionHost) {
    AVLOGV("pokeAHole");
    struct addrinfo hints, *result1, *result2 = NULL;
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    AString source;
    AString server_port;

    Vector<struct addrinfo*> s_addrinfos;

    if (GetAttribute(transport.c_str(), "source", &source)) {
        AVLOGI("found 'source' = %s field in Transport response",
            source.c_str());
        int err = getaddrinfo(source.c_str(), NULL, &hints, &result1);
        if (err != 0 || result1 == NULL || !isValidAddr(result1)) {
            AVLOGV("Failed to get the address of source");
            if (result1 != NULL)
                freeaddrinfo(result1);
        } else {
            AVLOGV("get the address of source");
            s_addrinfos.push(result1);
        }
    }

    int err = getaddrinfo(sessionHost.c_str(), NULL, &hints, &result2);
    if (err != 0 || result2 == NULL || !isValidAddr(result2)) {
        AVLOGV("Failed to look up address of session host '%s'",
                sessionHost.c_str());
        if (result2 != NULL)
            freeaddrinfo(result2);
    } else {
        AVLOGV("get the endpoint address of session host");
        s_addrinfos.push(result2);
    }

    if (s_addrinfos.size() == 0){
        AVLOGE("Failed to get any session address");
        return false;
    }

    if (!GetAttribute(transport.c_str(),
                         "server_port",
                         &server_port)) {
        AVLOGW("Missing 'server_port' field in Transport response.");
        return false;
    }

    int rtpPort, rtcpPort;
    if (sscanf(server_port.c_str(), "%d-%d", &rtpPort, &rtcpPort) != 2
            || rtpPort <= 0 || rtpPort > 65535
            || rtcpPort <=0 || rtcpPort > 65535
            || rtcpPort != rtpPort + 1) {
        AVLOGE("Server picked invalid RTP/RTCP port pair %s,"
            " RTP port must be even, RTCP port must be one higher.",
            server_port.c_str());

        return false;
    }

    if (rtpPort & 1) {
        AVLOGW("Server picked an odd RTP port, it should've picked an "
            "even one, we'll let it pass for now, but this may break "
            "in the future.");
    }

    // Make up an RR/SDES RTCP packet.
    sp<ABuffer> buf = new ABuffer(65536);
    buf->setRange(0, 0);
    MyHandler::addRR(buf);
    MyHandler::addSDES(rtpSocket, buf);

    for (uint32_t i = 0; i < s_addrinfos.size(); i++){
        size_t size = 0;
        if (!setAddrPortAndSize(s_addrinfos[i], rtpPort, &size)) {
            freeaddrinfo(s_addrinfos[i]);
            continue;
        }
        ssize_t n = sendto(
            rtpSocket, buf->data(), buf->size(), 0,
            s_addrinfos[i]->ai_addr, size);

        if (n < (ssize_t)buf->size()) {
            AVLOGE("failed to poke a hole for RTP packets");
            freeaddrinfo(s_addrinfos[i]);
            continue;
        }

        if (!setAddrPortAndSize(s_addrinfos[i], rtcpPort, &size)) {
            freeaddrinfo(s_addrinfos[i]);
            continue;
        }
        n = sendto(
            rtcpSocket, buf->data(), buf->size(), 0,
            s_addrinfos[i]->ai_addr, size);

        if (n < (ssize_t)buf->size()) {
            AVLOGE("failed to poke a hole for RTCP packets");
            freeaddrinfo(s_addrinfos[i]);
            continue;
        }

        freeaddrinfo(s_addrinfos[i]);
        AVLOGI("successfully poked holes for the address");
    }
    return true;
}

void ExtendedServiceUtils::makePortPair(int *rtpSocket, int *rtcpSocket, unsigned *rtpPort,
        bool isIPV6) {
    AVLOGV("MakePortPair isIPV6 = %d", isIPV6);
    *rtpSocket = socket(isIPV6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
    CHECK_GE(*rtpSocket, 0);
    bumpSocketBufferSize(*rtpSocket, isIPV6);

    *rtcpSocket = socket(isIPV6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
    CHECK_GE(*rtcpSocket, 0);

    bumpSocketBufferSize(*rtcpSocket, isIPV6);

    unsigned portRangeStart = 0;
    unsigned portRangeEnd = 0;
    getRtpPortRange(&portRangeStart, &portRangeEnd);
    unsigned start = (unsigned)((random() % (portRangeEnd - portRangeStart))
            + portRangeStart);
    start &= ~1;
    if (start < portRangeStart) {
        start += 2;
    }
    AVLOGV("rtp port range [%u, %u]", start, portRangeEnd);

    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
    for (unsigned port = start; port <= portRangeEnd; port += 2) {
        if (isIPV6) {
            memset(&addr_in6, 0, sizeof(addr_in6));
            addr_in6.sin6_family = AF_INET6;
            addr_in6.sin6_addr = in6addr_any;
            addr_in6.sin6_port = htons(port);
        } else {
            memset(&addr_in, 0, sizeof(addr_in));
            addr_in.sin_family = AF_INET;
            addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
            addr_in.sin_port = htons(port);
        }

        struct sockaddr *addr = isIPV6 ? (struct sockaddr *)&addr_in6
                : (struct sockaddr *)&addr_in;

        if (bind(*rtpSocket, addr, isIPV6 ? sizeof(struct sockaddr_in6)
                : sizeof(struct sockaddr_in)) < 0) {
            continue;
        }
        if (isIPV6)
            addr_in6.sin6_port = htons(port + 1);
        else
            addr_in.sin_port = htons(port+1);

        if (bind(*rtcpSocket, addr, isIPV6 ? sizeof(struct sockaddr_in6)
                : sizeof(struct sockaddr_in)) == 0) {
            *rtpPort = port;
            AVLOGV("rtpPort: %u", port);
            return;
        }
    }
    TRESPASS();
}

const char* ExtendedServiceUtils::parseURL(AString *host) {
    AVLOGV("ParseURL");
    ssize_t bracketBegin = host->find("[");
    if (bracketBegin != 0)
        return AVMediaServiceUtils::parseURL(host);

    AVLOGV("Parse IPV6 ip address: %s", host->c_str());
    ssize_t bracketEnd = host->find("]");
    if (bracketEnd <= 0) {
        AVLOGE("failed to parse IPV6");
        return NULL;
    }

    // If there is a port present, leave it for parsing in ParseURL
    // otherwise, remove all trailing characters in the hostname
    size_t trailing = host->size() - bracketEnd;
    bool isRemoved = false;
    if (host->find(":", bracketEnd) == bracketEnd + 1) {
        // 2 characters must be subtracted to account for the removal of
        // the starting and ending brackets below --> bracketEnd + 1 - 2
        isRemoved = true;
        trailing = 1;
    }
    const char *colonPos = isRemoved ? (host->c_str() + bracketEnd - 1)
            : NULL;
    host->erase(bracketEnd, trailing);
    host->erase(0, 1);
    AVLOGV("found IPV6 ip address");
    return colonPos;
}

void ExtendedServiceUtils::bumpSocketBufferSize(int s, bool isIPV6) {
    int size = 256 * 1024;
    if (isIPV6)
        CHECK_EQ(setsockopt(s, IPPROTO_IPV6, IPV6_RECVPKTINFO, &size, sizeof(size)), 0);
    else
        CHECK_EQ(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)), 0);
}

bool ExtendedServiceUtils::isValidAddr(const struct addrinfo *addr) {
    if (addr == NULL) {
        AVLOGW("isValidAddr: addrinfo is NULL");
        return false;
    }
    uint32_t sin4addr;
    struct in6_addr* sin6addr;
    switch (addr->ai_family) {
        case AF_INET:
            sin4addr = ((struct sockaddr_in *)addr->ai_addr)->sin_addr.s_addr;
            if (sin4addr == INADDR_NONE || IN_LOOPBACK(sin4addr))
                return false;
            break;
        case AF_INET6:
            sin6addr = &(((struct sockaddr_in6 *)addr->ai_addr)->sin6_addr);
            if (IN6_IS_ADDR_UNSPECIFIED(sin6addr) || IN6_IS_ADDR_LOOPBACK(sin6addr))
                return false;
            break;
        default:
            AVLOGW("isValidAddr: unknown protocol family");
            return false;
    }
    return true;
}

bool ExtendedServiceUtils::setAddrPortAndSize(const struct addrinfo *addr,
            int port, size_t* size) {
    if (addr == NULL) {
        AVLOGW("setAddrPort: addrinfo is NULL");
        return false;
    }
    switch (addr->ai_family) {
        case AF_INET:
            ((struct sockaddr_in *)addr->ai_addr)->sin_port = htons(port);
            *size = sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            ((struct sockaddr_in6 *)addr->ai_addr)->sin6_port = htons(port);
            *size = sizeof(struct sockaddr_in6);
            break;
        default:
            AVLOGW("setAddrPort: unknown protocol family");
            return false;
    }
    return true;
}

bool ExtendedServiceUtils::parseTrackURL(AString url, AString val) {
    AVLOGV("parseTrackURL");
    return url.endsWith(val.c_str());
}

void ExtendedServiceUtils::appendRange(AString *request) {
    AVLOGV("appendRange");
    if (request == NULL)
        return;
    request->append(AStringPrintf("Range: npt=0-\r\n"));
}

void ExtendedServiceUtils::setServerTimeoutUs(int64_t timeout) {
    AVLOGV("setServerTimeoutUs: %" PRId64, timeout);
    mServerTimeoutUs = timeout;
}

void ExtendedServiceUtils::appendMeta(media::Metadata *meta) {
    if (meta == NULL)
        return;
    if (mServerTimeoutUs > -1) {
        AVLOGV("appendMeta");
        meta->appendInt32(8801, (int32_t)(mServerTimeoutUs / 1000));
        mServerTimeoutUs = -1;
    }
}

bool ExtendedServiceUtils::checkNPTMapping(uint32_t *rtpInfoTime, int64_t *playTimeUs,
            bool *nptValid, uint32_t rtpTime) {
    AVLOGV("checkNPTMapping");
    *rtpInfoTime = rtpTime;
    *playTimeUs = 0ll;
    *nptValid = true;
    return true;
}

void ExtendedServiceUtils::addH263AdvancedPacket(const sp<ABuffer> &buffer,
        List<sp<ABuffer>> *packets, uint32_t rtpTime) {
    if (packets == NULL)
        return;
    AVLOGV("addAdvancedPacket");
    uint32_t time;
    if (buffer->meta()->findInt32("rtp-time", (int32_t *)&time)
            && rtpTime == time && packets->size() > 0
            && buffer->size() >= 2) {
        AVLOGV("insert the rtp packet into the candidate access unit");
        unsigned payloadHeader = U16_AT(buffer->data());
        unsigned p = (payloadHeader >> 10) & 1;
        unsigned v = (payloadHeader >> 9) & 1;
        unsigned plen = (payloadHeader >> 3) & 0x3f;
        unsigned pebit = payloadHeader & 7;
        if (v != 0u || plen != 0u || pebit != 0u) {
            AVLOGE("Malformed packet");
            return;
        }
        size_t skip = v + plen + (p ? 0 : 2);
        buffer->setRange(buffer->offset() + skip, buffer->size() - skip);
        if (p) {
            buffer->data()[0] = 0x00;
            buffer->data()[1] = 0x00;
        }
        uint32_t seqNum = (uint32_t)buffer->int32Data();
        List<sp<ABuffer> >::iterator it = packets->begin();
        while (it != packets->end() && (uint32_t)(*it)->int32Data() < seqNum){
            ++it;
        }

        if (it != packets->end() && (uint32_t)(*it)->int32Data() == seqNum) {
            AVLOGE("Discarding duplicate buffer in packets");
            return;
        }
        AVLOGV("insert the buffer into the current packets");
        packets->insert(it, buffer);
    }
}

bool ExtendedServiceUtils::parseNTPRange(const char *s, float *npt1, float *npt2) {
    AVLOGV("parseNTPRange");
    if (s[0] == '-') {
        return false;
    }

    if (!strncmp("now", s, 3)) {
        return false;
    }

    char *end;
    *npt1 = strtof(s, &end);

    if (end == NULL || end == s || *end != '-') {
        return false;
    }

    s = end + 1;

    if (*s == '\0') {
        *npt2 = FLT_MAX;
        return false;
    }

    if (!strncmp("now", s, 3)) {
        return false;
    }

    *npt2 = strtof(s, &end);

    if (end == NULL || end == s || *end != '\0') {
        return false;
    }
    AVLOGV("parseNTPRange: %f - %f", *npt1, *npt2);
    return *npt2 > *npt1;
}

void ExtendedServiceUtils::getRtpPortRange(unsigned *start, unsigned *end) {
    char value[PROPERTY_VALUE_MAX];
    if (!property_get("persist.vendor.sys.media.rtp-ports", value, NULL)
            || (sscanf(value, "%u-%u", start, end) != 2)
            || (*start > *end || *start <= 1024 || *end >= 65535)) {
        AVLOGV("Cannot get port range from property, use the default numbers");
        *start = 15550;
        *end = 65535;
    }
}

ExtendedServiceUtils::ExtendedServiceUtils()
    : mServerTimeoutUs(-1) {
    updateLogLevel();
    AVLOGV("ExtendedServiceUtils()");
}

ExtendedServiceUtils::~ExtendedServiceUtils() {
    AVLOGV("~ExtendedServiceUtils()");
}

}
