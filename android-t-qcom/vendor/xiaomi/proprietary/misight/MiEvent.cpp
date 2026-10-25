//
// Created by ritter on 22-3-8.
//

#define LOG_TAG "MiEvent"

#include "MiEvent.h"

#include <json/json.h>
#include <utils/Log.h>

namespace android {
class MiEvent::MiEventPayload
{
public:
    Json::Value mJson;
	uint32_t mKeyCnt;
    MiEventPayload(): mKeyCnt(0)
    {
        mJson.clear();
    }
    virtual ~MiEventPayload()
    {
        mJson.clear();
    }
    template<typename T>
    void put(const std::string& key, const T& val)
    {
        if (mKeyCnt > MAX_PARM_NUM) {
            return ;
        }
        mKeyCnt++;
        mJson[key.substr(0, MAX_KEY_LEN)] = val;
    }

    template<typename T>
    void putArray(const std::string& key, const std::vector<T>& array)
    {
        if (mKeyCnt > MAX_PARM_NUM) {
            return;
        }
        mKeyCnt++;

        int valSize = array.size();
        if (valSize == 0) {
            return;
        }

        Json::Value jsonArray;
        valSize = (valSize > MAX_ARR_SIZE ? MAX_ARR_SIZE : valSize);
        for (size_t i = 0; i < valSize; ++i) {
            Json::Value element(array[i]);
            jsonArray.append(element);
        }
        mJson[key.substr(0, MAX_KEY_LEN)] = jsonArray;
    }

    void putMiEvent(const std::string& key, MiEvent& miEvent)
    {
        if (mKeyCnt > MAX_PARM_NUM) {
            return;
        }
        mKeyCnt++;

        mJson[key.substr(0, MAX_KEY_LEN)] = miEvent.mPayload->mJson;
    }

    void putMiEvent(const std::string& key, std::vector<MiEvent>& miEventArray)
    {
        int valSize = miEventArray.size();
        if (valSize == 0) {
            return;
        }

        if (mKeyCnt > MAX_PARM_NUM) {
            return;
        }
        mKeyCnt++;

        Json::Value jsonArray;
        valSize = (valSize > MAX_ARR_SIZE ? MAX_ARR_SIZE : valSize);
        for (size_t i = 0; i < valSize; ++i) {
            jsonArray.append(miEventArray[i].mPayload->mJson);
        }
        if (!jsonArray.empty()) {
            mJson[key.substr(0, MAX_KEY_LEN)] = jsonArray;
        }
    }
};

MiEvent::MiEvent(uint32_t eventID)
    : mEventID(eventID), mNowSec(getNowSeconds()), mPayload(new MiEventPayload())
{
	ALOGI("MiEvent: this %p, paylod=%p", this, this->mPayload);
}


MiEvent::~MiEvent()
{
    ALOGI("~MiEvent: this %p, paylod=%p", this, this->mPayload);
    delete mPayload;
}

MiEvent::MiEvent(const MiEvent& miEvent) noexcept
{
    copyMiEvent(miEvent);
}

MiEvent::MiEvent(const MiEvent&& miEvent) noexcept
{
    copyMiEvent(miEvent);
}

MiEvent& MiEvent::operator=(const MiEvent& miEvent) {
    copyMiEvent(miEvent);
    return *this;
}

MiEvent& MiEvent::operator=(const MiEvent&& miEvent)
{
    copyMiEvent(miEvent);
    return *this;
}

void MiEvent::copyMiEvent(const MiEvent& miEvent)
{
    if (&miEvent == this) {
        return;
    }
    this->mEventID = miEvent.mEventID;
    this->mNowSec = miEvent.mNowSec;
    this->mPayload = new MiEventPayload();
    this->mPayload->mJson = miEvent.mPayload->mJson;
	this->mPayload->mKeyCnt = miEvent.mPayload->mKeyCnt;
	ALOGI("copyMiEvent: this %p, paylod=%p", this, this->mPayload);
}

const std::string MiEvent::flatten()
{
    std::string output = "EventId ";
    output.append(std::to_string(mEventID));
    output.append(" -t ");
    output.append(std::to_string(mNowSec));
    output.append(" -paraList ");
    output.append(flattenJson());
    ALOGD("flatten %s", output.c_str());
    return output;
}

const std::string MiEvent::jsonFormatString()
{
    return mPayload->mJson.toStyledString();
}

// private methods
const std::string MiEvent::flattenJson()
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = ""; // If you want whitespace-less output
    builder.settings_["precision"] = 3; // float precision
    const std::string output = Json::writeString(builder, mPayload->mJson);
    return output;
}

uint32_t MiEvent::getNowSeconds()
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec;
}

MiEvent& MiEvent::putString(const std::string& key, const std::string& val)
{
    mPayload->put<std::string>(key, val.substr(0, MAX_VALUE_LEN));
    return *this;
}

MiEvent& MiEvent::putString(const std::string& key, const std::vector<std::string>& val)
{
    int valSize = val.size();
    if (valSize == 0) {
        return *this;
    }

    std::vector<std::string> strVec;
    valSize = (valSize > MAX_ARR_SIZE ? MAX_ARR_SIZE : valSize);
    for (size_t i = 0; i < valSize; ++i) {
        strVec.push_back(val[i].substr(0, MAX_VALUE_LEN));
    }
    mPayload->putArray<std::string>(key, strVec);
    return *this;
}

MiEvent& MiEvent::putBool(const std::string& key, bool val)
{
    mPayload->put<bool>(key, val);
    return *this;
}

MiEvent& MiEvent::putBool(const std::string& key, const std::vector<bool>& val)
{
    mPayload->putArray<bool>(key, val);
	return *this;
}


MiEvent& MiEvent::putInt(const std::string& key, int val)
{
    mPayload->put<int>(key, val);
	return *this;
}

MiEvent& MiEvent::putInt(const std::string& key, const std::vector<int>& val)
{
    mPayload->putArray<int>(key, val);
    return *this;
}

MiEvent& MiEvent::putShort(const std::string& key, short val)
{
    mPayload->put<short>(key, val);
    return *this;
}

MiEvent& MiEvent::putShort(const std::string& key, const std::vector<short>& val)
{
    mPayload->putArray<short>(key, val);
    return *this;
}

MiEvent& MiEvent::putFloat(const std::string& key, float val)
{
    mPayload->put<float>(key, val);
    return *this;
}

MiEvent& MiEvent::putFloat(const std::string& key, const std::vector<float>& val)
{
    mPayload->putArray<float>(key, val);
    return *this;
}

MiEvent& MiEvent::putLong(const std::string& key, int64_t val)
{
    mPayload->put<int64_t>(key, val);
    return *this;
}

MiEvent& MiEvent::putLong(const std::string& key, const std::vector<int64_t>& val)
{
    mPayload->putArray<int64_t>(key, val);
    return *this;
}

MiEvent& MiEvent::putMiEvent(const std::string& key, std::vector<MiEvent>& miEventArray)
{
    mPayload->putMiEvent(key, miEventArray);
    return *this;
}

MiEvent& MiEvent::putMiEvent(const std::string& key, MiEvent& val)
{
    mPayload->putMiEvent(key, val);
    return *this;
}

bool MiEvent::setPayload(const std::string& payload)
{
    bool parseSucceeded = false;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errorMessage;
    parseSucceeded = reader->parse(&*payload.begin(), &*payload.end(), &(mPayload->mJson), &errorMessage);
    if (!parseSucceeded) {
        ALOGE("convert %s to json fail, %s", payload.c_str(), errorMessage.c_str());
    }
    return parseSucceeded;
}

} //namespace android