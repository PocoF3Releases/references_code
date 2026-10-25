//
// Created by ritter on 22-3-8.
//

#define LOG_TAG "MiSight"

#include "MiSight.h"

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <utils/Log.h>

#define MIEV_DIR "/dev/miev"

namespace android {

int MiSight::sendEvent(MiEvent& event) {
    return writeToKernel(event.flatten());
}

int MiSight::sendEvent(int eventId, const std::string& eventStr)
{
    MiEvent miEvent = MiEvent(eventId);
	if (miEvent.setPayload(eventStr)) {
        return writeToKernel(miEvent.flatten());
	} else {
	    return -1;
	}

}

int MiSight::writeToKernel(const std::string& str) {
    int fd = open(MIEV_DIR, O_WRONLY);
    if (fd < 0) {
        ALOGE("writeToKernel:%s fail", str.c_str());
        return -1;
    }
    int ret = write(fd, str.c_str(), str.size());
    if (ret < 0) {
        ALOGE("writeToKernel:%s fail", str.c_str());
    } else {
        ALOGD("writeToKernel:%s", str.c_str());
    }

    close(fd);
    return ret;
}
/*
//private method
bool MiSight::parseJsonFromString(const std::string& fromJsonStr, Json::Value& toJsonValue) {
    bool parseSucceeded = false;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errorMessage;
    parseSucceeded = reader->parse(&*fromJsonStr.begin(), &*fromJsonStr.end(), &toJsonValue, &errorMessage);
    return parseSucceeded;
}
*/
} //namespace android