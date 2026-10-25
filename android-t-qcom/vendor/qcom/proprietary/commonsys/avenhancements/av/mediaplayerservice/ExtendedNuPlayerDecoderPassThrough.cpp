/*
 * Copyright (c) 2015-2016, 2019, Qualcomm Technologies, Inc.
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
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedNuPlayerDecoderPassThrough"
#include <common/AVLog.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MetaData.h>
#include <QCMetaData.h>

#include <nuplayer/include/nuplayer/NuPlayer.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderBase.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderPassThrough.h>
#include <nuplayer/include/nuplayer/NuPlayerSource.h>
#include <nuplayer/include/nuplayer/NuPlayerRenderer.h>

#include "mediaplayerservice/AVNuExtensions.h"
#include "stagefright/AVExtensions.h"
#include "mediaplayerservice/ExtendedNuPlayerDecoderPassThrough.h"

namespace android {

ExtendedNuPlayerDecoderPassThrough::ExtendedNuPlayerDecoderPassThrough(
        const sp<AMessage> &notify,
        const sp<NuPlayer::Source> &source,
        const sp<NuPlayer::Renderer> &renderer)
    : NuPlayer::DecoderPassThrough(notify, source, renderer),
      mAudioFormat(AUDIO_FORMAT_INVALID),
      mVorbisHdrRequired(true),
      mVorbisHdrCommitted(true) {
    AVLOGV("ExtendedNuPlayerDecoderPassThrough()");
}

sp<ABuffer> ExtendedNuPlayerDecoderPassThrough::aggregateBuffer(
        const sp<ABuffer> &accessUnit) {
    sp<ABuffer> aggregate;

    if (mAudioFormat != AUDIO_FORMAT_VORBIS) {
        return NuPlayer::DecoderPassThrough::aggregateBuffer(accessUnit);
    }

    /*------ FOR VORBIS FORMAT ONLY ------*/
    if (accessUnit == NULL) {
        // accessUnit is saved to mPendingAudioAccessUnit
        // return current mAggregateBuffer
        aggregate = mAggregateBuffer;
        mAggregateBuffer.clear();
        mVorbisHdrCommitted = true;
        return aggregate;
    }

    size_t smallSize = accessUnit->size();
    if ((mAggregateBuffer == NULL)
            // Don't bother if only room for a few small buffers.
            && (smallSize < (mAggregateBufferSizeBytes / 3))) {
        // Create a larger buffer for combining smaller buffers from the extractor.
        mAggregateBuffer = new ABuffer(mAggregateBufferSizeBytes);
        mAggregateBuffer->setRange(0, 0); // start empty
    }

    // assemble vorbis header for the anchor buffer
    if (mVorbisHdrRequired) {
        sp<MetaData> audioMeta = mSource->getFormatMeta(true /* audio */);
        mVorbisHdrBuffer = assembleVorbisHdr(audioMeta);
        CHECK(mVorbisHdrBuffer != NULL);

        size_t tmpSize = mVorbisHdrBuffer->size() + smallSize;
        mAnchorBuffer = new ABuffer(tmpSize);

        int32_t frameSize = smallSize - sizeof(int32_t);
        memcpy(mAnchorBuffer->base(), mVorbisHdrBuffer->data(), mVorbisHdrBuffer->size());
        memcpy(mAnchorBuffer->base() + mVorbisHdrBuffer->size(), &frameSize, sizeof(int32_t));
        memcpy(mAnchorBuffer->base() + mVorbisHdrBuffer->size() + sizeof(int32_t),
                accessUnit->data(), frameSize);

        mAnchorBuffer->setRange(0, tmpSize);
        mVorbisHdrRequired = false;
        // Don't clear mVorbisHdrBuffer right away, because it's still of use.
        // Lazy destroy instead.

        // Most unlikely mAggregateBuffer is not empty here.
        // Reset its content in case uncaught corner case happens..
        if ((mAggregateBuffer != NULL) && (mAggregateBuffer->size() > 0)) {
            AVLOGE("mAggregateBuffer is not empty!");
            mAggregateBuffer->setRange(0, 0);
        }
    }

    if (mAggregateBuffer != NULL) {
        int64_t timeUs;
        int64_t dummy;
        bool smallTimestampValid = accessUnit->meta()->findInt64("timeUs", &timeUs);
        bool bigTimestampValid = mAggregateBuffer->meta()->findInt64("timeUs", &dummy);
        // Will the smaller buffer fit?
        size_t bigSize = mAggregateBuffer->size();
        size_t roomLeft = mAggregateBuffer->capacity() - bigSize;

        // Return anchor buffer directly if room left is not sufficient.
        if ((mAnchorBuffer != NULL) &&
            (roomLeft <= mAnchorBuffer->size())) {
            AVLOGI("aggregate buffer room can't hold access unit with vorbis header");

            // mAnchorBuffer only appears as a first small buffer,
            // but the first small buffer isn't necessarily an anchor buffer.
            CHECK(bigSize == 0);
            if (smallTimestampValid) {
                mAnchorBuffer->meta()->setInt64("timeUs", timeUs);
            }

            mPendingAudioErr = OK;
            mPendingAudioAccessUnit = accessUnit;

            aggregate = mAnchorBuffer;
            mAnchorBuffer.clear();
            mAggregateBuffer.clear();
            mVorbisHdrBuffer.clear();
            return aggregate;
        }

        // Should we save this small buffer for the next big buffer?
        // If the first small buffer did not have a timestamp then save
        // any buffer that does have a timestamp until the next big buffer.
        if ((smallSize > roomLeft)
            || (!bigTimestampValid && (bigSize > 0) && smallTimestampValid)) {
            mPendingAudioErr = OK;
            mPendingAudioAccessUnit = accessUnit;
            aggregate = mAggregateBuffer;
            mAggregateBuffer.clear();
            mVorbisHdrCommitted = true;
        } else {
            // Grab time from first small buffer if available.
            if ((bigSize == 0) && smallTimestampValid) {
                mAggregateBuffer->meta()->setInt64("timeUs", timeUs);
            }

            // Append small buffer to the bigger buffer.
            if (mAnchorBuffer != NULL) {
                // prepend vorbis header whenever available
                CHECK(bigSize == 0);
                memcpy(mAggregateBuffer->base(), mAnchorBuffer->data(), mAnchorBuffer->size());
                mAggregateBuffer->setRange(0, mAnchorBuffer->size());
                bigSize += mVorbisHdrBuffer->size();
                mVorbisHdrBuffer.clear();
                mAnchorBuffer.clear();
                mVorbisHdrCommitted = false;
            } else {
                // Convert frame stream to adapt to dsp vorbis decoder.
                // Trim 4bytes appended, and prepend 4bytes required (assume one frame per buffer).
                // NOTE: extra data is used for decode error recovery at the less frame drop possible.
                int32_t frameSize = smallSize - sizeof(int32_t);
                memcpy(mAggregateBuffer->base() + bigSize, &frameSize, sizeof(int32_t));
                memcpy(mAggregateBuffer->base() + bigSize + sizeof(int32_t),
                        accessUnit->data(), frameSize);
            }
            bigSize += smallSize;
            mAggregateBuffer->setRange(0, bigSize);

            AVLOGV("feedDecoderInputData() smallSize = %zu, bigSize = %zu, capacity = %zu",
                    smallSize, bigSize, mAggregateBuffer->capacity());
        }
    } else {
        // decided not to aggregate
        if (mAnchorBuffer != NULL) {
            aggregate = mAnchorBuffer;
            mAnchorBuffer.clear();
            mVorbisHdrBuffer.clear();
        } else {
            sp<ABuffer> transBuffer = new ABuffer(smallSize);
            int32_t frameSize = smallSize - sizeof(int32_t);
            memcpy(transBuffer->base(), &frameSize, sizeof(int32_t));
            memcpy(transBuffer->base() + sizeof(int32_t),
                    accessUnit->data(), frameSize);

            transBuffer->setRange(0, smallSize);
            aggregate = transBuffer;
        }

        int64_t timeUs;
        if (accessUnit->meta()->findInt64("timeUs", &timeUs)) {
            aggregate->meta()->setInt64("timeUs", timeUs);
        }
    }

    return aggregate;
}

void ExtendedNuPlayerDecoderPassThrough::onResume(bool notifyComplete) {
    // always resend vorbis header upon resume
    if(mAudioFormat == AUDIO_FORMAT_VORBIS) {
        mVorbisHdrRequired = true;
    }

    NuPlayer::DecoderPassThrough::onResume(notifyComplete);
}

void ExtendedNuPlayerDecoderPassThrough::onConfigure(const sp<AMessage> &format) {
    sp<MetaData> audioMeta = mSource->getFormatMeta(true /* audio */);

    if (AVNuUtils::get()->isVorbisFormat(audioMeta)) {
        mAudioFormat = AUDIO_FORMAT_VORBIS;
    }

    NuPlayer::DecoderPassThrough::onConfigure(format);
}

void ExtendedNuPlayerDecoderPassThrough::onFlush() {
    // aggregation buffer will be discarded without being rendered
    // if vorbis header isn't yet committed, need to resend it again in next round
    if ((mAudioFormat == AUDIO_FORMAT_VORBIS) && !mVorbisHdrCommitted) {
        mVorbisHdrRequired = true;
    }

    NuPlayer::DecoderPassThrough::onFlush();
}

ExtendedNuPlayerDecoderPassThrough::~ExtendedNuPlayerDecoderPassThrough() {
    AVLOGV("~ExtendedNuPlayerDecoderPassThrough()");
}

sp<ABuffer> ExtendedNuPlayerDecoderPassThrough::assembleVorbisHdr(const sp<MetaData> &meta) {
    size_t vorbisHdrSize = 0;
    sp<ABuffer> vorbisHdrBuffer = NULL;
    const size_t MAX_META_SIZE = 0;
    const void *hdrDat1, *hdrDat2, *hdrDat3;
    size_t hdrSize1 = 0, hdrSize2 = 0, hdrSize3 = 0;
    uint32_t type;

    if (meta == NULL) {
        AVLOGE("invalid meta data");
        return NULL;
    }

    if (meta->findData(kKeyOpaqueCSD0, &type, &hdrDat1, &hdrSize1)) {
        vorbisHdrSize += hdrSize1;
        vorbisHdrSize += 4;
    }

    if (meta->findData(kKeyVorbisData, &type, &hdrDat2, &hdrSize2)) {
        // intentally truncate the 2nd packet, as dsp tolerates the missing of it.
        if (hdrSize2 > MAX_META_SIZE) {
            hdrSize2 = 0;
        }
        vorbisHdrSize += hdrSize2;
        vorbisHdrSize += 4;
    } else {
        // fill vacant header even if parser doesn't provide kKeyVorbisData.
        hdrSize2 = 0;
        vorbisHdrSize += 4;
    }

    if (meta->findData(kKeyOpaqueCSD1, &type, &hdrDat3, &hdrSize3)) {
        vorbisHdrSize += hdrSize3;
        vorbisHdrSize += 4;
    }

    // assemble vorbis header
    if (vorbisHdrSize > 0) {
        size_t offset = 0;
        vorbisHdrBuffer = new ABuffer(vorbisHdrSize);
        vorbisHdrBuffer->setRange(0, 0);

        if (hdrSize1 > 0) {
            memcpy(vorbisHdrBuffer->base(), (int32_t *)&hdrSize1, sizeof(int32_t));
            memcpy(vorbisHdrBuffer->base() + sizeof(int32_t), hdrDat1, hdrSize1);
            offset += (hdrSize1 + sizeof(int32_t));
        }
        if (1 /* hdrSize2 >= 0 */) {
            memcpy(vorbisHdrBuffer->base() + offset, (int32_t *)&hdrSize2, sizeof(int32_t));
            memcpy(vorbisHdrBuffer->base() + offset + sizeof(int32_t), hdrDat2, hdrSize2);
            offset += (hdrSize2 + sizeof(int32_t));
        }
        if (hdrSize3 > 0) {
            memcpy(vorbisHdrBuffer->base() + offset, (int32_t *)&hdrSize3, sizeof(int32_t));
            memcpy(vorbisHdrBuffer->base() + offset + sizeof(int32_t), hdrDat3, hdrSize3);
            offset += (hdrSize3 + sizeof(int32_t));
        }
        vorbisHdrBuffer->setRange(0, offset);
    }

    return vorbisHdrBuffer;
}

}

