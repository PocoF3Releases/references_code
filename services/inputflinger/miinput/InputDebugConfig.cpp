#define LOG_TAG "InputDebugConfig"

#include "InputDebugConfig.h"
#include <android-base/stringprintf.h>

#define INDENT2 "    "

using android::base::StringPrintf;

// MIUI ADD:
namespace android {

static inline const char* toString(bool value) {
    return value ? "true" : "false";
}

InputDebugConfig::InputDebugConfig(bool inputDispatcherMajor, bool inputDispatcherDetail, bool inputDispatcherAll, bool inputReaderAll, bool inputTransportAll)
    : mInputDispatcherMajor(inputDispatcherMajor),
      mInputDispatcherDetail(inputDispatcherDetail),
      mInputDispatcherAll(inputDispatcherAll),
      mInputReaderAll(inputReaderAll),
      mInputTransportAll(inputTransportAll){}

status_t InputDebugConfig::readFromParcel(const Parcel& parcel, sp<InputDebugConfig> debugInputDebugConfig){
    bool inputDispatcherMajor;
    bool inputDispatcherDetail;
    bool inputDispatcherAll;
    bool inputReaderAll;
    bool inputTransportAll;

    SAFE_PARCEL(parcel.readBool, &inputDispatcherMajor);
    SAFE_PARCEL(parcel.readBool, &inputDispatcherDetail);
    SAFE_PARCEL(parcel.readBool, &inputDispatcherAll);
    SAFE_PARCEL(parcel.readBool, &inputReaderAll);
    SAFE_PARCEL(parcel.readBool, &inputTransportAll);

    int32_t result = 0;
    if(debugInputDebugConfig->mInputTransportAll != inputTransportAll){
        result |= CONFIG_INPUT_TRANSPORT;
    }

    debugInputDebugConfig->mInputDispatcherMajor = inputDispatcherMajor;
    if (inputDispatcherAll) {
        debugInputDebugConfig->mInputDispatcherDetail = inputDispatcherAll;
    } else {
        debugInputDebugConfig->mInputDispatcherDetail = inputDispatcherDetail;
    }
    debugInputDebugConfig->mInputDispatcherAll = inputDispatcherAll;
    debugInputDebugConfig->mInputReaderAll = inputReaderAll;
    debugInputDebugConfig->mInputTransportAll = inputTransportAll;

    return result;
}


void InputDebugConfig::dump(std::string& dump) {
    dump += "\nInputDebugConfig State\n";
    dump += StringPrintf(INDENT2 "mInputDispatcherMajor: %s\n",
                            toString(mInputDispatcherMajor));
    dump += StringPrintf(INDENT2 "mInputDispatcherDetail: %s\n",
                                toString(mInputDispatcherDetail));
    dump += StringPrintf(INDENT2 "mInputDispatcherAll: %s\n",
                                toString(mInputDispatcherAll));
    dump += StringPrintf(INDENT2 "mInputReaderAll: %s\n",
                                toString(mInputReaderAll));
    dump += StringPrintf(INDENT2 "mInputTransportAll: %s\n",
                                toString(mInputTransportAll));
}

}