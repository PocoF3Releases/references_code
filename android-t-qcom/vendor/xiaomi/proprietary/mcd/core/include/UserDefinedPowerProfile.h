#ifndef USER_DEFINED_POWER_PROFILE_H
#define USER_DEFINED_POWER_PROFILE_H

#include <utils/String8.h>
#include <utils/KeyedVector.h>

#include "octvm.h"
#include "utils/cJSON.h"
#include "drv/platform_power.h"

using namespace android;

class UserDefinedPowerProfile {
public:
    UserDefinedPowerProfile(String8 profileName, struct power_tunable_params *param_profile);
    ~UserDefinedPowerProfile();

    String8 getProfileName();
    bool matchPowerProfileByName(String8 name);

private:
    String8 profileName;
    struct power_tunable_params *param_profile;

    bool writeToFile(const char *filePath);

public:
    static void removeAllPowerProfile();
    static bool addUserPowerProfile(cJSON *configData, bool strict);
    static void addUserPowerProfile(UserDefinedPowerProfile *profile);
    static UserDefinedPowerProfile *checkUserPowerProfile(String8 name);

private:
    static KeyedVector<String8, UserDefinedPowerProfile *> *userProfileList;
};

#endif