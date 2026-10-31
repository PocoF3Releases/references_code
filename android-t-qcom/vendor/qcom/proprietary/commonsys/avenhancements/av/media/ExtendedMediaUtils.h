/*
 * Copyright (c) 2015-2019, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _EXTENDED_MEDIA_UTILS_H_
#define _EXTENDED_MEDIA_UTILS_H_

namespace android {

struct ExtendedMediaUtils : public AVMediaUtils {

    virtual size_t AudioTrackGetOffloadFrameCount(size_t frameCount);

    virtual bool AudioTrackIsTrackOffloaded(audio_io_handle_t);
#ifndef BRINGUP_WIP
    virtual sp<MediaRecorder> createMediaRecorder(const String16& opPackageName);
#endif

    // ----- NO TRESSPASSING BEYOND THIS LINE ------
protected:
    virtual ~ExtendedMediaUtils();

private:
    ExtendedMediaUtils(const ExtendedMediaUtils&);
    ExtendedMediaUtils &operator=(const ExtendedMediaUtils&);
public:
    ExtendedMediaUtils();
};

extern "C" AVMediaUtils *createExtendedMediaUtils();

} //namespace android

#endif //_EXTENDED_MEDIA_UTILS_H_

