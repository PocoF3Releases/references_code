/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "RecentEventLogger.h"
#include "SensorServiceUtils.h"

#include <android/util/ProtoOutputStream.h>
#include <frameworks/base/core/proto/android/service/sensor_service.proto.h>
#include <utils/Timers.h>

#include <inttypes.h>
#include "MiSensorServiceStub.h"

namespace android {
namespace SensorServiceUtil {

#ifdef MI_SENSORSERVICE_FEATURE_ENABLE
#define SENSOR_TYPE_BACK_LUX 33171055  // back light sensor type
#define SENSOR_TYPE_LIGHT_SECONDARY 33171081  // sencondary light sensor type
#define SENSOR_TYPE_FOLD_STATUS 33171087 // fold status sensor type
#endif

namespace {
    constexpr size_t LOG_SIZE = 10;
    constexpr size_t LOG_SIZE_MED = 30;  // debugging for slower sensors
    constexpr size_t LOG_SIZE_LARGE = 50;  // larger samples for debugging
#ifdef MI_SENSORSERVICE_FEATURE_ENABLE
    constexpr size_t LOG_SIZE_HUGE = 1000;  // huge samples for debugging
#endif
}// unnamed namespace

RecentEventLogger::RecentEventLogger(int sensorType) :
        mSensorType(sensorType), mEventSize(eventSizeBySensorType(mSensorType)),
        mRecentEvents(logSizeBySensorType(sensorType)), mMaskData(false),
        mIsLastEventCurrent(false) {
    // blank
}

void RecentEventLogger::addEvent(const sensors_event_t& event) {
    std::lock_guard<std::mutex> lk(mLock);
    mRecentEvents.emplace(event);
    mIsLastEventCurrent = true;
    MiSensorServiceStub::handleProxUsageTime(event, 0);
    MiSensorServiceStub::handlePocModeUsageTime(event, 0);
    MiSensorServiceStub::handleSensorEvent(event);
}

bool RecentEventLogger::isEmpty() const {
    return mRecentEvents.size() == 0;
}

void RecentEventLogger::setLastEventStale() {
    std::lock_guard<std::mutex> lk(mLock);
    mIsLastEventCurrent = false;
}

std::string RecentEventLogger::dump() const {
    std::lock_guard<std::mutex> lk(mLock);

    //TODO: replace String8 with std::string completely in this function
    String8 buffer;

    buffer.appendFormat("last %zu events\n", mRecentEvents.size());
    int j = 0;
    for (int i = mRecentEvents.size() - 1; i >= 0; --i) {
        const auto& ev = mRecentEvents[i];
        struct tm * timeinfo = localtime(&(ev.mWallTime.tv_sec));
        buffer.appendFormat("\t%2d (ts=%.9f, wall=%02d:%02d:%02d.%03d) ",
                ++j, ev.mEvent.timestamp/1e9, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                (int) ns2ms(ev.mWallTime.tv_nsec));

        // data
        if (!mMaskData) {
            if (mSensorType == SENSOR_TYPE_STEP_COUNTER) {
                buffer.appendFormat("%" PRIu64 ", ", ev.mEvent.u64.step_counter);
            } else {
                for (size_t k = 0; k < mEventSize; ++k) {
                    buffer.appendFormat("%.2f, ", ev.mEvent.data[k]);
                }
            }
        } else {
            buffer.append("[value masked]");
        }
        buffer.append("\n");
    }
    return std::string(buffer.string());
}

/**
 * Dump debugging information as android.service.SensorEventsProto protobuf message using
 * ProtoOutputStream.
 *
 * See proto definition and some notes about ProtoOutputStream in
 * frameworks/base/core/proto/android/service/sensor_service.proto
 */
void RecentEventLogger::dump(util::ProtoOutputStream* proto) const {
    using namespace service::SensorEventsProto;
    std::lock_guard<std::mutex> lk(mLock);

    proto->write(RecentEventsLog::RECENT_EVENTS_COUNT, int(mRecentEvents.size()));
    for (int i = mRecentEvents.size() - 1; i >= 0; --i) {
        const auto& ev = mRecentEvents[i];
        const uint64_t token = proto->start(RecentEventsLog::EVENTS);
        proto->write(Event::TIMESTAMP_SEC, float(ev.mEvent.timestamp) / 1e9f);
        proto->write(Event::WALL_TIMESTAMP_MS, ev.mWallTime.tv_sec * 1000LL
                + ns2ms(ev.mWallTime.tv_nsec));

        if (mMaskData) {
            proto->write(Event::MASKED, true);
        } else {
            if (mSensorType == SENSOR_TYPE_STEP_COUNTER) {
                proto->write(Event::INT64_DATA, int64_t(ev.mEvent.u64.step_counter));
            } else {
                for (size_t k = 0; k < mEventSize; ++k) {
                    proto->write(Event::FLOAT_ARRAY, ev.mEvent.data[k]);
                }
            }
        }
        proto->end(token);
    }
}

void RecentEventLogger::setFormat(std::string format) {
    if (format == "mask_data" ) {
        mMaskData = true;
    } else {
        mMaskData = false;
    }
}

bool RecentEventLogger::populateLastEventIfCurrent(sensors_event_t *event) const {
    std::lock_guard<std::mutex> lk(mLock);

    if (mIsLastEventCurrent && mRecentEvents.size()) {
        // Index 0 contains the latest event emplace()'ed
        *event = mRecentEvents[0].mEvent;
        return true;
    } else {
        return false;
    }
}


size_t RecentEventLogger::logSizeBySensorType(int sensorType) {
    if (sensorType == SENSOR_TYPE_STEP_COUNTER ||
        sensorType == SENSOR_TYPE_SIGNIFICANT_MOTION ||
#ifdef MI_SENSORSERVICE_FEATURE_ENABLE
        sensorType == SENSOR_TYPE_ACCELEROMETER) {
#else
        sensorType == SENSOR_TYPE_ACCELEROMETER ||
        sensorType == SENSOR_TYPE_LIGHT) {
#endif
        return LOG_SIZE_LARGE;
    }
    if (sensorType == SENSOR_TYPE_PROXIMITY) {
        return LOG_SIZE_MED;
    }
#ifdef MI_SENSORSERVICE_FEATURE_ENABLE
    if (sensorType == SENSOR_TYPE_LIGHT ||
        sensorType == SENSOR_TYPE_LIGHT_SECONDARY ||
        sensorType == SENSOR_TYPE_FOLD_STATUS ||
        sensorType == SENSOR_TYPE_BACK_LUX) {
        return LOG_SIZE_HUGE;
    }
#endif
    return LOG_SIZE;
}

RecentEventLogger::SensorEventLog::SensorEventLog(const sensors_event_t& e) : mEvent(e) {
    clock_gettime(CLOCK_REALTIME, &mWallTime);
}

} // namespace SensorServiceUtil
} // namespace android
