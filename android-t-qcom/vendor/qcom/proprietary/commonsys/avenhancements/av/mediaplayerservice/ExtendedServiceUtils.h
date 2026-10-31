/*
 * Copyright (c) 2015, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _EXTENDED_SERVICE_UTILS_H_
#define _EXTENDED_SERVICE_UTILS_H_

#include <MediaPlayerFactory.h>
#include <common/AVLog.h>
#include <mediaplayerservice/AVMediaServiceExtensions.h>
#include <dlfcn.h>

namespace android {

struct ExtendedServiceUtils : public AVMediaServiceUtils {
    ExtendedServiceUtils();

    // RTSP utils
    virtual bool pokeAHole(sp<MyHandler> handler, int rtpSocket,
            int rtcpSocket, const AString &transport, const AString &sessionHost);
    virtual void makePortPair(int *rtpSocket, int *rtcpSocket, unsigned *rtpPort,
            bool isIPV6);
    virtual const char* parseURL(AString *host);

    // RTSP customization
    virtual bool parseTrackURL(AString url, AString val);
    virtual void appendRange(AString *request);
    virtual void setServerTimeoutUs(int64_t timeout);
    virtual void appendMeta(media::Metadata *meta);
    virtual bool checkNPTMapping(uint32_t *rtpInfoTime, int64_t *playTimeUs,
            bool *nptValid, uint32_t rtpTime);
    virtual void addH263AdvancedPacket(const sp<ABuffer> &buffer,
            List<sp<ABuffer>> *packets, uint32_t rtpTime);
    virtual bool parseNTPRange(const char *s, float *npt1, float *npt2);

protected:
    ~ExtendedServiceUtils();

private:
    int64_t mServerTimeoutUs;
    ExtendedServiceUtils(const ExtendedServiceUtils &);
    ExtendedServiceUtils &operator=(ExtendedServiceUtils &);
    void bumpSocketBufferSize(int s, bool isIPV6);
    bool isValidAddr(const struct addrinfo *addr);
    bool setAddrPortAndSize(const struct addrinfo *addr, int port, size_t *size);

    void getRtpPortRange(unsigned *start, unsigned *end);
};

extern "C" AVMediaServiceUtils *createExtendedMediaServiceUtils();

} // namespace android

#endif // _EXTENDED_SERVICE_UTILS_H_
