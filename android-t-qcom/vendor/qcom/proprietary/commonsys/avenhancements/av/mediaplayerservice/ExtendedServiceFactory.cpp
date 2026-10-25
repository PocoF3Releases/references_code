/*
 * Copyright (c) 2015,2021-2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedServiceFactory"
#include <common/AVLog.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/rtsp/ARTSPConnection.h>
#include <media/stagefright/rtsp/ARTPConnection.h>
#include <StagefrightRecorder.h>

#include "mediaplayerservice/AVMediaServiceExtensions.h"

#include "mediaplayerservice/ExtendedServiceFactory.h"
#include "mediaplayerservice/ExtendedSFRecorder.h"
#include "mediaplayerservice/ExtendedARTSPConnection.h"
#include "mediaplayerservice/ExtendedARTPConnection.h"

namespace android {

AVMediaServiceFactory *createExtendedMediaServiceFactory() {
    return new ExtendedServiceFactory;
}

StagefrightRecorder *ExtendedServiceFactory::createStagefrightRecorder(
        const AttributionSourceState& attributionSource) {
    return new ExtendedSFRecorder(attributionSource);
}

sp<ARTSPConnection> ExtendedServiceFactory::createARTSPConnection(
        bool uidValid, uid_t uid) {
    return new ExtendedARTSPConnection(uidValid, uid);
}


sp<ARTPConnection> ExtendedServiceFactory::createARTPConnection() {
    return new ExtendedARTPConnection();
}


ExtendedServiceFactory::ExtendedServiceFactory() {
    updateLogLevel();
    AVLOGV("ExtendedServiceFactory()");
}

ExtendedServiceFactory::~ExtendedServiceFactory() {
    AVLOGV("~ExtendedServiceFactory()");
}

}




