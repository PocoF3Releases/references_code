/*
 * Copyright (c) 2015-2016, 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef _EXTENDED_NU_UTILS_H_
#define _EXTENDED_NU_UTILS_H_

#include <common/AVConfigHelper.h>

namespace android {

struct ExtendedNuUtils : public AVNuUtils {

    virtual bool pcmOffloadException(const sp<AMessage> &format);

    bool isVorbisFormat(const sp<MetaData> &meta);
    bool flacSwSupports24Bit();
    bool avOffloadEnabled();

    virtual audio_format_t getPCMFormat(const sp<AMessage> &format);
    virtual void setMPEGHOutputFormat(int32_t binaural, int32_t channelMask);
    virtual void setCodecOutputFormat(const sp<AMessage> &format);
    virtual bool isByteStreamModeEnabled(const sp<MetaData> &meta);

    virtual void printFileName(int fd);
    virtual uint32_t getFlags();
    virtual bool canUseSetBuffers(const sp<MetaData> &Meta);
    virtual void overWriteAudioOutputFormat(
            sp <AMessage> &dst, const sp <AMessage> &src);
    virtual bool dropCorruptFrame();
    // ----- NO TRESSPASSING BEYOND THIS LINE ------
protected:
    virtual ~ExtendedNuUtils();

private:
    ExtendedNuUtils(const ExtendedNuUtils&);
    ExtendedNuUtils &operator=(const ExtendedNuUtils&);
    bool findAndReplaceKeys(const char *key,
            sp <AMessage> &dst, const sp<AMessage> &src);
    bool mDropCorruptFrame;
    int32_t mBinauralMode;
    int32_t mChannelMask;
    AVConfigHelper *pConfigsIns;
public:
    ExtendedNuUtils();
};

extern "C" AVNuUtils *createExtendedNuUtils();

} //namespace android

#endif //_EXTENDED_NU_UTILS_H_

