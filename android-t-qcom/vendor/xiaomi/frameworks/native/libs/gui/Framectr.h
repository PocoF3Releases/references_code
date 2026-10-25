
#ifndef FRAMECTR_H
#define FRAMECTR_H
#include <utils/Errors.h>

enum
{
    FBC_QUEUE_B,
    FBC_QUEUE_E,
    FBC_DEQUEUE_B,
    FBC_DEQUEUE_E,
    FBC_CNT,
    FBC_DISCNT,
};

void FbcInit();
android::status_t NotifyFbc(const int32_t& value, const uint64_t& bufID, const int32_t& param, const uint64_t& id);

#endif