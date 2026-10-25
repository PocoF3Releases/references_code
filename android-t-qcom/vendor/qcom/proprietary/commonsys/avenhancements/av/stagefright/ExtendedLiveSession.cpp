/*
 * Copyright (c) 2015-2017, 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "ExtendedLiveSession"
#include <common/AVLog.h>

#include <ID3.h>
#include <media/IMediaHTTPService.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <M3UParser.h>
#include <mpeg2ts/AnotherPacketSource.h>
#include "ExtendedLiveSession.h"
#include "ExtendedPlaylistFetcher.h"
#include <media/stagefright/MetaData.h>

#ifdef HLS_AUDIO_ONLY_IMG_DISPLAY
extern "C" {
    #include "jpeglib.h"
    #include "jerror.h"
}
#endif

namespace android {

const char *PATH = "/data/misc/media/defaultimage.jpeg";

ExtendedLiveSession::ExtendedLiveSession(
        const sp<AMessage> &notify,
        uint32_t flags,
        const sp<IMediaHTTPService> &httpService,
        uint32_t featureFlag)
    : LiveSession(notify, flags, httpService),
      mFeatureFlag(featureFlag),
      mIsImageDisplay(false),
      mDownloadFirstTs(false) {
    AVLOGV("create extended LiveSession");
}

ExtendedLiveSession::~ExtendedLiveSession() {
    AVLOGV("~ExtendedLiveSession");
}

void ExtendedLiveSession::onMessageReceived(const sp<AMessage> &msg) {
    switch(msg->what()) {
        case kWhatDetectImage:
        {
            AVLOGV("kWhatDetectImage");
            sp<AnotherPacketSource> source = mPacketSources.valueFor(
                    LiveSession::STREAMTYPE_VIDEO);
            status_t result = OK;
            if (source != NULL && source->hasBufferAvailable(&result)) {
                AVLOGV("video buffer is available, wait for next time");
                msg->post(100000);
                break;
            }
            sp<ABuffer> buffer;
            source = mPacketSources.valueFor(
                    LiveSession::STREAMTYPE_METADATA);
            result = OK;
            if (source == NULL || !source->hasBufferAvailable(&result)
                    || source->dequeueAccessUnit(&buffer) != OK
                    || buffer == NULL) {
                AVLOGV("meta data has not been queued in the source");
                if (!mIsImageDisplay) {
                    onDefaultMetaDataDetected();
                }
                msg->post(100000);
                break;
            }
            onMetaDataDetected(buffer);
            source->requeueAccessUnit(buffer);
            break;
        }

        case kWhatSwitchConfiguration:
        {
            AVLOGV("kWhatSwitchConfiguration");
            onSwitchConfiguration(msg);
            break;
        }

        case kWhatSwitchRealBandwidth:
        {
            AVLOGV("kWhatSwitchRealBandwidth");
            sp<AReplyToken> replyID;
            msg->senderAwaitsResponse(&replyID);

            int32_t switchNeeded = (int32_t)onSwitchToRealBandwidth();

            sp<AMessage> response = new AMessage;
            response->setInt32("switchNeeded", switchNeeded);
            response->postReply(replyID);
            break;
        }

        default:
            LiveSession::onMessageReceived(msg);
            break;
    }
}

sp<PlaylistFetcher> ExtendedLiveSession::addFetcher(const char *uri) {
    AVLOGV("addFetcher");
    ssize_t index = mFetcherInfos.indexOfKey(uri);

    if (index >= 0) {
        return NULL;
    }

    sp<AMessage> notify = new AMessage(kWhatFetcherNotify, this);
    notify->setString("uri", uri);
    notify->setInt32("switchGeneration", mSwitchGeneration);

    FetcherInfo info;
    info.mFetcher = new ExtendedPlaylistFetcher(
            notify, this, uri, mCurBandwidthIndex,
            mSubtitleGeneration, mDownloadFirstTs);
    info.mDurationUs = -1ll;
    info.mToBeRemoved = false;
    info.mToBeResumed = false;
    mFetcherLooper->registerHandler(info.mFetcher);

    mFetcherInfos.add(uri, info);

    return info.mFetcher;
}

status_t ExtendedLiveSession::dequeueAccessUnit(StreamType stream,
        sp<ABuffer> *accessUnit) {
    status_t err = LiveSession::dequeueAccessUnit(stream, accessUnit);
    if (mFeatureFlag & IMAGE_SUPPORT) {
        if (stream == STREAMTYPE_AUDIO && err == INFO_DISCONTINUITY
                && isAudioOnly()) {
            AVLOGV("switch to audio only clip, detect the image");
            sp<AMessage> msg = new AMessage(kWhatDetectImage, this);
            msg->post();
        }
    }
    return err;
}

void ExtendedLiveSession::swapPacketSource(StreamType stream) {
    AVLOGV("[%s] swapPacketSource", getNameForStream(stream));

    // transfer packets from source2 to source
    sp<AnotherPacketSource> &aps = mPacketSources.editValueFor(stream);
    sp<AnotherPacketSource> &aps2 = mPacketSources2.editValueFor(stream);

    // queue discontinuity in mPacketSource
    if (stream == STREAMTYPE_VIDEO && mIsImageDisplay) {
        // if image is displayed in native window, force format change
        // to reconfig native window during recreating video decoder
        sp<AMessage> extra = new AMessage();
        extra->setInt32("force-formatChange", 1);
        mIsImageDisplay = false;
        aps->queueDiscontinuity(ATSParser::DISCONTINUITY_FORMAT_ONLY, extra, false);
    } else {
        aps->queueDiscontinuity(ATSParser::DISCONTINUITY_FORMAT_ONLY, NULL, false);
    }

    // queue packets in mPacketSource2 to mPacketSource
    status_t finalResult = OK;
    sp<ABuffer> accessUnit;
    while (aps2->hasBufferAvailable(&finalResult) && finalResult == OK &&
          OK == aps2->dequeueAccessUnit(&accessUnit)) {
        aps->queueAccessUnit(accessUnit);
    }
    aps2->clear();
}

void ExtendedLiveSession::postPrepared(status_t err) {
    AVLOGV("postPrepared");
    LiveSession::postPrepared(err);
    if (mFeatureFlag & IMAGE_SUPPORT) {
        if (isAudioOnly() && err == OK) {
            AVLOGV("detect image in audio only clip");
            sp<AMessage> msg = new AMessage(kWhatDetectImage, this);
            msg->post();
        }
    }
}

bool ExtendedLiveSession::isAudioOnly() {
    sp<MetaData> videoMeta;
    sp<MetaData> audioMeta;
    if (getStreamFormatMeta(LiveSession::STREAMTYPE_VIDEO, &videoMeta) != OK
            && getStreamFormatMeta(LiveSession::STREAMTYPE_AUDIO, &audioMeta) == OK) {
        AVLOGV("audio only clip");
        return true;
    }
    return false;
}

void ExtendedLiveSession::onDefaultMetaDataDetected() {
    AVLOGV("onDefaultMetaDataDetected");
    FILE* source = fopen(PATH, "rb");
    if (source == NULL) {
        AVLOGD("no default image was found");
        return;
    }
    int32_t width;
    int32_t height;
    sp<ABuffer> buffer = decodeJpegImage(source, &width, &height);
    if (buffer != NULL) {
        notifyImage(buffer, width, height);
        mIsImageDisplay = true;
    }
    fclose(source);
}

void ExtendedLiveSession::onMetaDataDetected(const sp<ABuffer> &buffer) {
    AVLOGV("onMetaDataDetected");
    ID3 id3(buffer->data(), buffer->size());
    if (!id3.isValid()) {
        return;
    }
    size_t imageSize;
    String8 mime;
    const void *imageData = id3.getAlbumArt(&imageSize, &mime);
    if (imageSize == 0) {
        return;
    }
    AVLOGV("found the album image (%s) with ID3 format in meta data", mime.string());
    if (strcasecmp(mime.string(), MEDIA_MIMETYPE_IMAGE_JPEG) != 0) {
        AVLOGE("only support image/jpeg");
        return;
    }
    sp<ABuffer> imageBuffer = ABuffer::CreateAsCopy(imageData, imageSize);
    if (imageBuffer == NULL) {
        AVLOGE("image buffer is null");
        return;
    }
    FILE* source = fmemopen(imageBuffer->data(), imageSize, "rb");
    if (source == NULL) {
        return;
    }
    int32_t width;
    int32_t height;
    sp<ABuffer> rgbBuffer = decodeJpegImage(source, &width, &height);
    if (rgbBuffer != NULL) {
        notifyImage(rgbBuffer, width, height);
        mIsImageDisplay = true;
    }
    fclose(source);
}

sp<ABuffer> ExtendedLiveSession::decodeJpegImage(FILE* source, int32_t *width, int32_t *height) {
    if (source == NULL) {
        return NULL;
    }
#ifdef HLS_AUDIO_ONLY_IMG_DISPLAY
    AVLOGV("decode jpeg to rgb565");
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, source);
    if (JPEG_HEADER_OK != jpeg_read_header(&cinfo, true)) {
        AVLOGE("failed to decode jpeg header");
        jpeg_destroy_decompress(&cinfo);
        return NULL;
    }
    cinfo.out_color_space = JCS_RGB565;
    if (!jpeg_start_decompress(&cinfo)) {
        AVLOGE("failed to decompress jpeg picture");
        jpeg_destroy_decompress(&cinfo);
        return NULL;
    }
    AVLOGV("picture width = %d, height = %d", cinfo.output_width, cinfo.output_height);
    size_t stride = cinfo.output_width * 2;
    size_t dataSize = stride * cinfo.output_height;
    sp<ABuffer> outBuffer = new ABuffer(dataSize);
    for (size_t i = 0; i < cinfo.output_height; i++) {
        JSAMPLE* rowptr = (JSAMPLE*)(outBuffer->data() + stride * i);
        int32_t row_count = jpeg_read_scanlines(&cinfo, &rowptr, 1);
        if (row_count == 0) {
            AVLOGV("row_count = 0");
            cinfo.output_scanline = cinfo.output_height;
            break;
        }
    }
    *width = cinfo.output_width;
    *height = cinfo.output_height;
    AVLOGV("finish decoding jpeg");
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return outBuffer;
#else
    *width = 0;
    *height = 0;
    return NULL;
#endif
}

void ExtendedLiveSession::notifyImage(const sp<ABuffer> &buffer, int32_t width, int32_t height) {
    sp<AMessage> notify = mNotify->dup();
    notify->setInt32("what", kWhatImageDetected);
    notify->setBuffer("image-buffer", buffer);
    notify->setInt32("image-width", width);
    notify->setInt32("image-height", height);
    notify->post();
}

status_t ExtendedLiveSession::switchToRealBandwidth() {
    sp<AMessage> msg = new AMessage(kWhatSwitchRealBandwidth, this);

    sp<AMessage> response;
    status_t err = msg->postAndAwaitResponse(&response);

    int32_t switchNeeded = 0;
    if (err == OK && response != NULL) {
        CHECK(response->findInt32("switchNeeded", &switchNeeded));
    }

    return switchNeeded;
}

bool ExtendedLiveSession::onSwitchToRealBandwidth() {
    int32_t bandwidthBps = 0, shortTermBps;
    bool isStable;
    //TODO:: work in progress Android-O
    //       API is removed, modify logic accordingly
    //mBandwidthEstimator->estimateBandwidth(&bandwidthBps, &isStable, &shortTermBps);
    ssize_t bandwidthIndex = getBandwidthIndex(bandwidthBps);
    mDownloadFirstTs = false;
    if (mCurBandwidthIndex != bandwidthIndex) {
        AVLOGV("switching to RealBandwidth...");
        sp<AMessage> msg = new AMessage(kWhatSwitchConfiguration, this);
        msg->setInt32("bandwidthindex", (int32_t)bandwidthIndex);
        msg->post();
        AVLOGV("Posting switchConfiguration to change bandwidth to index %zd on the first play",
                bandwidthIndex);
        return true;
    }
    AVLOGV("No need to switch bandwidth");
    return false;
}

void ExtendedLiveSession::onSwitchConfiguration(const sp<AMessage> &msg) {
    int32_t bandwidthIndex;
    msg->findInt32("bandwidthindex", &bandwidthIndex);
    changeConfiguration(
            mInPreparationPhase ? 0 : -1ll, bandwidthIndex);
}

void ExtendedLiveSession::onMasterPlaylistFetched(const sp<AMessage> &msg) {
    sp<M3UParser> playlist;
    CHECK(msg->findObject("playlist", (sp<RefBase> *)&playlist));
    if ((mFeatureFlag & ACTUAL_BANDWIDTH)
            && playlist != NULL
            && playlist->isVariantPlaylist()) {
            mDownloadFirstTs = true;
    }
    LiveSession::onMasterPlaylistFetched(msg);
}

void ExtendedLiveSession::onPollBuffering() {
    AVLOGV("onPollBuffering: mSwitchInProgress %d, mReconfigurationInProgress %d, "
            "mInPreparationPhase %d, mCurBandwidthIndex %zd, mStreamMask 0x%x",
        mSwitchInProgress, mReconfigurationInProgress,
        mInPreparationPhase, mCurBandwidthIndex, mStreamMask);

    bool underflow, ready, down, up;
    if (checkBuffering(underflow, ready, down, up)) {
        if (mInPreparationPhase) {
            // Allow down switch even if we're still preparing.
            //
            // Some streams have a high bandwidth index as default,
            // when bandwidth is low, it takes a long time to buffer
            // to ready mark, then it immediately pauses after start
            // as we have to do a down switch. It's better experience
            // to restart from a lower index, if we detect low bw.

            // Additional check is added for sending postPrepared
            // notification. The check(!mDownloadFirstTs) ensures
            // that postPrepared is not sent for the first ts segment
            // which is used for estimating the b/w.
            if (!switchBandwidthIfNeeded(false /* up */, down) && ready && !mDownloadFirstTs) {
                postPrepared(OK);
            }
        }

        if (!mInPreparationPhase) {
            if (ready) {
                stopBufferingIfNecessary();
            } else if (underflow) {
                startBufferingIfNecessary();
            }
            switchBandwidthIfNeeded(up, down);
        }
    }

    schedulePollBuffering();
}

size_t ExtendedLiveSession::getBandwidthIndex(int32_t bandwidthBps) {
    if (mFeatureFlag & ASYM_BANDWIDTH) {
        if (mBandwidthItems.size() < 2) {
            AVLOGV("getBandwidthIndex() called for single bandwidth playlist");
            return 0;
        }
        ssize_t index = mBandwidthItems.size() - 1;
        ssize_t lowestBandwidth = getLowestValidBandwidthIndex();
        size_t relativeBandwidth = mBandwidthItems[lowestBandwidth].mBandwidth;
        size_t adjustedBandwidthBps = bandwidthBps * (mDownloadFirstTs ? 8 : 9) / 10;
        while (index > lowestBandwidth) {
            const BandwidthItem &item = mBandwidthItems[index];
            if (item.mBandwidth <= adjustedBandwidthBps
                    && isBandwidthValid(item)) {
                relativeBandwidth = item.mBandwidth;
                break;
            }
            --index;
        }
        if (mDownloadFirstTs) {
            AVLOGV("choose the index directly in the first time");
        } else {
            // in up switch case, if the real estimated bandwidth is NOT 30%
            // above the target bandwidth, index should move to a lower level.
            if (index > mCurBandwidthIndex
                    && (size_t)bandwidthBps <= relativeBandwidth * 13 / 10) {
                --index;
            }
        }
        AVLOGV("get bandwidth index = %zd", index);
        CHECK_GE(index, 0);
        return index;
    } else {
        return LiveSession::getBandwidthIndex(bandwidthBps);
    }
}

}  // namespace android
