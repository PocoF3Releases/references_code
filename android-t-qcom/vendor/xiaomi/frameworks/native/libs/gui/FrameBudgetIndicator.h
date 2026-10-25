#ifndef MTKFBC_H
#define MTKFBC_H
#include <utils/Errors.h>

enum
{
    FBC_QUEUE_BEG,
};

void fbcInit();
// void fbcDestroy();
android::status_t notifyFbc(const int32_t& value);

#endif