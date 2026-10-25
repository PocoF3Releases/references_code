/*
 * Copyright (C) 2012 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "codec"
#include <inttypes.h>
#include <utils/Log.h>

#include "SimplePlayer.h"
//MIUI ADD: start MIAUDIO_OZO
#include "WaveWriter.h"
#include <MediaStub.h>
//MIUI ADD: end

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <mediadrm/ICrypto.h>
#include <media/IMediaHTTPService.h>
#include <media/IMediaPlayerService.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/NuMediaExtractor.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <ui/DisplayMode.h>
#include <cutils/properties.h>

//MIUI ADD: start MIAUDIO_OZO
static const char *ozoGuidanceTag = "vendor.ozoaudio.guidance.value";
static const char *ozoOrientationTag = "vendor.ozoaudio.orientation.value";
static const bool kSupportOZO = property_get_bool("ro.vendor.audio.zoom.support", false );
//MIUI ADD: end

static void usage(const char *me) {
    fprintf(stderr, "usage: %s [-a] use audio\n"
                    "\t\t[-v] use video\n"
                    "\t\t[-p] playback\n"
                    "\t\t[-S] allocate buffers from a surface\n"
                    "\t\t[-R] render output to surface (enables -S)\n"
                    "\t\t[-T] use render timestamps (enables -R)\n"
//MIUI ADD: start MIAUDIO_OZO
                    "\t\t[-o] use OZO audio decoder\n"
                    "\t\t[-O file] write decoded audio to specified file\n"
                    "\t\t[-g guidance] set OZO guidance mode\n"
                    "\t\t[-l orientations] set OZO listener orientation, format [frame_ind yaw pitch roll]\n"
//MIUI ADD: end
                    , me);
    exit(1);
}

namespace android {

struct CodecState {
    sp<MediaCodec> mCodec;
    Vector<sp<MediaCodecBuffer> > mInBuffers;
    Vector<sp<MediaCodecBuffer> > mOutBuffers;
    bool mSignalledInputEOS;
    bool mSawOutputEOS;
    int64_t mNumBuffersDecoded;
    int64_t mNumBytesDecoded;
    bool mIsAudio;
};

}  // namespace android

//MIUI ADD: start MIAUDIO_OZO
static int decode(
        const android::sp<android::ALooper> &looper,
        const char *path,
        bool useAudio,
        bool useVideo,
        const android::sp<android::Surface> &surface,
        bool renderSurface,
        bool useTimestamp,
        const char *output,
        bool isOzoAudio,
        const char *ozoGuidance,
        android::KeyedVector<size_t, android::AString> & ozoEvents,
        android::KeyedVector<size_t, android::AString> & ozoHeadsetEvents) {
//MIUI ADD: end
    using namespace android;

    static int64_t kTimeout = 500ll;

//MIUI ADD: start MIAUDIO_OZO
    WaveWriter *waveWriter = nullptr;
//MIUI ADD: end

    sp<NuMediaExtractor> extractor = new NuMediaExtractor(NuMediaExtractor::EntryPoint::OTHER);
    if (extractor->setDataSource(NULL /* httpService */, path) != OK) {
        fprintf(stderr, "unable to instantiate extractor.\n");
        return 1;
    }

    KeyedVector<size_t, CodecState> stateByTrack;

    bool haveAudio = false;
    bool haveVideo = false;
    for (size_t i = 0; i < extractor->countTracks(); ++i) {
        sp<AMessage> format;
        status_t err = extractor->getTrackFormat(i, &format);
        CHECK_EQ(err, (status_t)OK);

        AString mime;
//MIUI ADD: start MIAUDIO_OZO
        if (isOzoAudio) {
            format->setString("mime", MEDIA_MIMETYPE_AUDIO_OZOAUDIO);
        }

        if (ozoGuidance) {
            format->setString(ozoGuidanceTag, ozoGuidance);
        }

        ssize_t eventIndex = ozoEvents.indexOfKey(0);
        if (eventIndex >= 0) {
            format->setString(ozoOrientationTag, ozoEvents.valueAt(eventIndex));
        }
//MIUI ADD: end
        CHECK(format->findString("mime", &mime));

        bool isAudio = !strncasecmp(mime.c_str(), "audio/", 6);
        bool isVideo = !strncasecmp(mime.c_str(), "video/", 6);

        if (useAudio && !haveAudio && isAudio) {
            haveAudio = true;
        } else if (useVideo && !haveVideo && isVideo) {
            haveVideo = true;
        } else {
            continue;
        }

        ALOGV("selecting track %zu", i);

        err = extractor->selectTrack(i);
        CHECK_EQ(err, (status_t)OK);

        CodecState *state =
            &stateByTrack.editValueAt(stateByTrack.add(i, CodecState()));

        state->mNumBytesDecoded = 0;
        state->mNumBuffersDecoded = 0;
        state->mIsAudio = isAudio;

        state->mCodec = MediaCodec::CreateByType(
                looper, mime.c_str(), false /* encoder */);

        CHECK(state->mCodec != NULL);

        err = state->mCodec->configure(
                format, isVideo ? surface : NULL,
                NULL /* crypto */,
                0 /* flags */);

        CHECK_EQ(err, (status_t)OK);

        state->mSignalledInputEOS = false;
        state->mSawOutputEOS = false;
    }

    CHECK(!stateByTrack.isEmpty());

    int64_t startTimeUs = android::ALooper::GetNowUs();
    int64_t startTimeRender = -1;

    for (size_t i = 0; i < stateByTrack.size(); ++i) {
        CodecState *state = &stateByTrack.editValueAt(i);

        sp<MediaCodec> codec = state->mCodec;

        CHECK_EQ((status_t)OK, codec->start());

        CHECK_EQ((status_t)OK, codec->getInputBuffers(&state->mInBuffers));
        CHECK_EQ((status_t)OK, codec->getOutputBuffers(&state->mOutBuffers));

        ALOGV("got %zu input and %zu output buffers",
              state->mInBuffers.size(), state->mOutBuffers.size());
    }

    bool sawInputEOS = false;

    for (;;) {
        if (!sawInputEOS) {
            size_t trackIndex;
            status_t err = extractor->getSampleTrackIndex(&trackIndex);

            if (err != OK) {
                ALOGV("saw input eos");
                sawInputEOS = true;
            } else {
                CodecState *state = &stateByTrack.editValueFor(trackIndex);

                size_t index;
                err = state->mCodec->dequeueInputBuffer(&index, kTimeout);

                if (err == OK) {
                    ALOGV("filling input buffer %zu", index);

                    const sp<MediaCodecBuffer> &buffer = state->mInBuffers.itemAt(index);
                    sp<ABuffer> abuffer = new ABuffer(buffer->base(), buffer->capacity());

                    err = extractor->readSampleData(abuffer);
                    CHECK_EQ(err, (status_t)OK);
                    buffer->setRange(abuffer->offset(), abuffer->size());

                    int64_t timeUs;
                    err = extractor->getSampleTime(&timeUs);
                    CHECK_EQ(err, (status_t)OK);

                    uint32_t bufferFlags = 0;

                    err = state->mCodec->queueInputBuffer(
                            index,
                            0 /* offset */,
                            buffer->size(),
                            timeUs,
                            bufferFlags);

                    CHECK_EQ(err, (status_t)OK);

                    extractor->advance();
                } else {
                    CHECK_EQ(err, -EAGAIN);
                }
            }
        } else {
            for (size_t i = 0; i < stateByTrack.size(); ++i) {
                CodecState *state = &stateByTrack.editValueAt(i);

                if (!state->mSignalledInputEOS) {
                    size_t index;
                    status_t err =
                        state->mCodec->dequeueInputBuffer(&index, kTimeout);

                    if (err == OK) {
                        ALOGV("signalling input EOS on track %zu", i);

                        err = state->mCodec->queueInputBuffer(
                                index,
                                0 /* offset */,
                                0 /* size */,
                                0ll /* timeUs */,
                                MediaCodec::BUFFER_FLAG_EOS);

                        CHECK_EQ(err, (status_t)OK);

                        state->mSignalledInputEOS = true;
                    } else {
                        CHECK_EQ(err, -EAGAIN);
                    }
                }
            }
        }

        bool sawOutputEOSOnAllTracks = true;
        for (size_t i = 0; i < stateByTrack.size(); ++i) {
            CodecState *state = &stateByTrack.editValueAt(i);
            if (!state->mSawOutputEOS) {
                sawOutputEOSOnAllTracks = false;
                break;
            }
        }

        if (sawOutputEOSOnAllTracks) {
            break;
        }

        for (size_t i = 0; i < stateByTrack.size(); ++i) {
            CodecState *state = &stateByTrack.editValueAt(i);

            if (state->mSawOutputEOS) {
                continue;
            }

            size_t index;
            size_t offset;
            size_t size;
            int64_t presentationTimeUs;
            uint32_t flags;
            status_t err = state->mCodec->dequeueOutputBuffer(
                    &index, &offset, &size, &presentationTimeUs, &flags,
                    kTimeout);

            if (err == OK) {
                ALOGV("draining output buffer %zu, time = %lld us",
                      index, (long long)presentationTimeUs);
//MIUI ADD: start MIAUDIO_OZO
                if (waveWriter)
                {
                    sp<MediaCodecBuffer> outputBuffer;
                    auto err = state->mCodec->getOutputBuffer(index, &outputBuffer);
                    CHECK_EQ(err, (status_t) OK);

                    waveWriter->Append(outputBuffer->data(), outputBuffer->size());
                }
//MIUI ADD: end
                ++state->mNumBuffersDecoded;
                state->mNumBytesDecoded += size;

                if (surface == NULL || !renderSurface) {
                    err = state->mCodec->releaseOutputBuffer(index);
                } else if (useTimestamp) {
                    if (startTimeRender == -1) {
                        // begin rendering 2 vsyncs (~33ms) after first decode
                        startTimeRender =
                                systemTime(SYSTEM_TIME_MONOTONIC) + 33000000
                                - (presentationTimeUs * 1000);
                    }
                    presentationTimeUs =
                            (presentationTimeUs * 1000) + startTimeRender;
                    err = state->mCodec->renderOutputBufferAndRelease(
                            index, presentationTimeUs);
                } else {
                    err = state->mCodec->renderOutputBufferAndRelease(index);
                }

                CHECK_EQ(err, (status_t)OK);

                if (flags & MediaCodec::BUFFER_FLAG_EOS) {
                    ALOGV("reached EOS on output.");

                    state->mSawOutputEOS = true;
                }
            } else if (err == INFO_OUTPUT_BUFFERS_CHANGED) {
                ALOGV("INFO_OUTPUT_BUFFERS_CHANGED");
                CHECK_EQ((status_t)OK,
                         state->mCodec->getOutputBuffers(&state->mOutBuffers));

                ALOGV("got %zu output buffers", state->mOutBuffers.size());
            } else if (err == INFO_FORMAT_CHANGED) {
                sp<AMessage> format;
                CHECK_EQ((status_t)OK, state->mCodec->getOutputFormat(&format));

                ALOGV("INFO_FORMAT_CHANGED: %s", format->debugString().c_str());
//MIUI ADD: start MIAUDIO_OZO
                if (!waveWriter && output) {
                    int32_t channelCount = 2;
                    int32_t sampleRate = 48000;
                    CHECK(format->findInt32("channel-count", &channelCount));
                    CHECK(format->findInt32("sample-rate", &sampleRate));
                    waveWriter = new WaveWriter(output, channelCount, sampleRate);
                }
//MIUI ADD: end
            } else {
                CHECK_EQ(err, -EAGAIN);
            }
//MIUI ADD: start MIAUDIO_OZO
            ssize_t eventIndex = ozoEvents.indexOfKey(state->mNumBuffersDecoded);
            if (eventIndex >= 0) {
                sp<AMessage> message = new AMessage();
                message->setString(ozoOrientationTag, ozoEvents.valueAt(eventIndex));
                state->mCodec->setParameters(message);
            }
//MIUI ADD: end
        }
    }

    int64_t elapsedTimeUs = android::ALooper::GetNowUs() - startTimeUs;

    for (size_t i = 0; i < stateByTrack.size(); ++i) {
        CodecState *state = &stateByTrack.editValueAt(i);

        CHECK_EQ((status_t)OK, state->mCodec->release());

        if (state->mIsAudio) {
            printf("track %zu: %lld bytes received. %.2f KB/sec\n",
                   i,
                   (long long)state->mNumBytesDecoded,
                   state->mNumBytesDecoded * 1E6 / 1024 / elapsedTimeUs);
        } else {
            printf("track %zu: %lld frames decoded, %.2f fps. %lld"
                    " bytes received. %.2f KB/sec\n",
                   i,
                   (long long)state->mNumBuffersDecoded,
                   state->mNumBuffersDecoded * 1E6 / elapsedTimeUs,
                   (long long)state->mNumBytesDecoded,
                   state->mNumBytesDecoded * 1E6 / 1024 / elapsedTimeUs);
        }
    }
//MIUI ADD: start MIAUDIO_OZO
    delete waveWriter;
//MIUI ADD: end
    return 0;
}

int main(int argc, char **argv) {
    using namespace android;

    const char *me = argv[0];

    bool useAudio = false;
    bool useVideo = false;
    bool playback = false;
    bool useSurface = false;
    bool renderSurface = false;
    bool useTimestamp = false;
//MIUI ADD: start MIAUDIO_OZO
    bool isOzoAudio = false;
    const char* ozoGuidance = "none";
    const char* ozoOrientations = nullptr;
    const char* ozoHeadsets = nullptr;
    const char* output = nullptr;

//MIUI ADD: end

    int res;
     while ((res = getopt(argc, argv, "havpSDRToH:O:g:l:")) >= 0) {
        switch (res) {
            case 'a':
            {
                useAudio = true;
                break;
            }
            case 'v':
            {
                useVideo = true;
                break;
            }
            case 'p':
            {
                playback = true;
                break;
            }
//MIUI ADD: start MIAUDIO_OZO
            case 'o':
            {
                isOzoAudio = true;
                break;
            }
            case 'O':
            {
                if(optarg) output = optarg;
                break;
            }
            case 'g':
            {
                if(optarg) ozoGuidance = optarg;
                break;
            }
            case 'H':
            {
                if(optarg) ozoHeadsets = optarg;
                break;
            }
            case 'l':
            {
                if(optarg) ozoOrientations = optarg;
                break;
            }
//MIUI ADD: end
            case 'T':
            {
                useTimestamp = true;
                FALLTHROUGH_INTENDED;
            }
            case 'R':
            {
                renderSurface = true;
                FALLTHROUGH_INTENDED;
            }
            case 'S':
            {
                useSurface = true;
                break;
            }
            case '?':
            case 'h':
            default:
            {
                usage(me);
            }
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 1) {
        usage(me);
    }

    if (!useAudio && !useVideo) {
        useAudio = useVideo = true;
    }
//MIUI ADD: start MIAUDIO_OZO
    // Orientation events for Ozo audio decoding
    if(kSupportOZO){
        KeyedVector<size_t, AString> ozoEvents;
        MediaStub::parseOzoOrientations(ozoEvents, ozoOrientations);

        // Headset (on/off) events for Ozo audio decoding
        KeyedVector<size_t, AString> ozoHeadsetEvents;
        MediaStub::parseOzoOrientations(ozoHeadsetEvents, ozoHeadsets);
    }
//MIUI ADD: end
    ProcessState::self()->startThreadPool();

    sp<android::ALooper> looper = new android::ALooper;
    looper->start();

    sp<SurfaceComposerClient> composerClient;
    sp<SurfaceControl> control;
    sp<Surface> surface;

    if (playback || (useSurface && useVideo)) {
        composerClient = new SurfaceComposerClient;
        CHECK_EQ(composerClient->initCheck(), (status_t)OK);

        const sp<IBinder> display = SurfaceComposerClient::getInternalDisplayToken();
        CHECK(display != nullptr);

        ui::DisplayMode mode;
        CHECK_EQ(SurfaceComposerClient::getActiveDisplayMode(display, &mode), NO_ERROR);

        const ui::Size& resolution = mode.resolution;
        const ssize_t displayWidth = resolution.getWidth();
        const ssize_t displayHeight = resolution.getHeight();

        ALOGV("display is %zd x %zd\n", displayWidth, displayHeight);

        control = composerClient->createSurface(
                String8("A Surface"),
                displayWidth,
                displayHeight,
                PIXEL_FORMAT_RGB_565,
                0);

        CHECK(control != NULL);
        CHECK(control->isValid());

        SurfaceComposerClient::Transaction{}
                 .setLayer(control, INT_MAX)
                 .show(control)
                 .apply();

        surface = control->getSurface();
        CHECK(surface != NULL);
    }

    if (playback) {
        sp<SimplePlayer> player = new SimplePlayer;
        looper->registerHandler(player);

        player->setDataSource(argv[0]);
        player->setSurface(surface->getIGraphicBufferProducer());
        player->start();
        sleep(60);
        player->stop();
        player->reset();
    } else {
//MIUI ADD: start MIAUDIO_OZO
        if(kSupportOZO){
            decode(looper, argv[0], useAudio, useVideo, surface, renderSurface,
               useTimestamp, output, isOzoAudio,
                ozoGuidance, ozoEvents, ozoHeadsetEvents);
//MIUI ADD: end
        } else {
            decode(looper, argv[0], useAudio, useVideo, surface, renderSurface,useTimestamp,NULL,NULL,NULL,NULL,NULL);
        }
    }

    if (playback || (useSurface && useVideo)) {
        composerClient->dispose();
    }

    looper->stop();

    return 0;
}
