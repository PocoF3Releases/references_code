//
// Created by ritter on 22-3-8.
//

#ifndef ANDROID_MIEVENT_H
#define ANDROID_MIEVENT_H

#include <iostream>

namespace android {
class MiEvent {
public:
    MiEvent(uint32_t eventID);

    virtual ~MiEvent();
    MiEvent(const MiEvent& miEvent) noexcept; // copy construct
    MiEvent& operator=(const MiEvent& miEvent); // = construct

    MiEvent(const MiEvent&& miEvent) noexcept; // move construct
    MiEvent& operator=(const MiEvent&& miEvent); // move = construct
    class MiEventPayload;

    MiEvent& putString(const std::string& key, const std::string& val);
    MiEvent& putString(const std::string& key, const std::vector<std::string>& val);

    MiEvent& putBool(const std::string& key, bool val);
    MiEvent& putBool(const std::string& key, const std::vector<bool>& val);

    MiEvent& putShort(const std::string& key, short val);
    MiEvent& putShort(const std::string& key, const std::vector<short>& val);

    MiEvent& putInt(const std::string& key, int val);
    MiEvent& putInt(const std::string& key, const std::vector<int>& val);

    MiEvent& putFloat(const std::string& key, float val);
    MiEvent& putFloat(const std::string& key, const std::vector<float>& val);

    MiEvent& putLong(const std::string& key, int64_t val);
    MiEvent& putLong(const std::string& key, const std::vector<int64_t>& val);

    MiEvent& putMiEvent(const std::string& key, std::vector<MiEvent>& miEventArray);
    MiEvent& putMiEvent(const std::string& key, MiEvent& val);

    const std::string jsonFormatString();
    const std::string flatten();
    bool setPayload(const std::string& eventStr);
    // Max length of payload's key
    static const size_t MAX_KEY_LEN = 32;

    // Max length of payload's value which type is string
    static const size_t MAX_VALUE_LEN = 128;

    // Max number of payload's param
    static const size_t MAX_PARM_NUM = 256;

    // Max size of payload's array
    static const size_t MAX_ARR_SIZE = 100;
private:
    const std::string flattenJson();
    uint32_t getNowSeconds();
    void copyMiEvent(const MiEvent& miEvent);

    uint32_t mEventID;
    uint32_t mNowSec;
    MiEventPayload *mPayload;
};
}; // namespace android
#endif //ANDROID_MIEVENT_H