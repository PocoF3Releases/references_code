#ifndef VENDOR_XIAOMI_HARDWARE_MISYS_CHECKPID_H
#define VENDOR_XIAOMI_HARDWARE_MISYS_CHECKPID_H

#include <utils/Log.h>
#include "cutils/properties.h"

#include <logwrap/logwrap.h>

#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include <vendor/xiaomi/hardware/misys/1.0/IMiSys.h>

int checkPidExists(int pid);

#endif  // VENDOR_XIAOMI_HARDWARE_MISYS_CHECKPID_H
