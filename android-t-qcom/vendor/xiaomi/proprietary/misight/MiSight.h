//
// Created by ritter on 22-3-8.
//

#ifndef ANDROID_MISIGHT_H
#define ANDROID_MISIGHT_H

#include "MiEvent.h"

#include <string>

namespace android {
class MiSight {
    public:
        static int sendEvent(MiEvent& event);
        static int sendEvent(int eventId, const std::string& eventStr);
    private:
        static int writeToKernel(const std::string& str);
};

extern "C" {
    void sendEventMisight(int eventId, const std::string& eventStr) {
        MiSight::sendEvent(eventId, eventStr);
    }
}

}; // namespace android

#endif //ANDROID_MISIGHT_H