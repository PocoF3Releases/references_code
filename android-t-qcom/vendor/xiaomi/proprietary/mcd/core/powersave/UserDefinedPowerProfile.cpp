#define LOG_TAG "octvm_power"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "UserDefinedPowerProfile.h"

using namespace android;

UserDefinedPowerProfile::UserDefinedPowerProfile(String8 profileName, struct power_tunable_params *param_profile)
{
    this->profileName = profileName;
    this->param_profile = param_profile;
}

UserDefinedPowerProfile::~UserDefinedPowerProfile()
{
    free(param_profile);
}

String8 UserDefinedPowerProfile::getProfileName()
{
    return profileName;
}

bool UserDefinedPowerProfile::matchPowerProfileByName(String8 name)
{
    return profileName == name;
}

bool UserDefinedPowerProfile::writeToFile(const char *filePath)
{
    FILE *fp = fopen(filePath, "w");
    if (fp == NULL) {
        fprintf(stderr, "open %s for write error, %s\n", filePath, strerror(errno));
        return false;
    }
    const char *param_path;
    if (param_profile->valid_param_flag & 0x1) {
        if ((param_path = get_cpu_param_path("cpufreq_min")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->cpufreq_min, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x2) {
        if ((param_path = get_cpu_param_path("cpufreq_max")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->cpufreq_max, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x4) {
        if ((param_path = get_cpu_param_path("cpufreq_gov")) != NULL) {
            fprintf(fp, "%s >%s\n", param_profile->cpufreq_gov, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x8) {
        if ((param_path = get_cpu_param_path("gpufreq_max")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->gpufreq_max, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x10) {
        if ((param_path = get_cpu_param_path("gpufreq_gov")) != NULL) {
            fprintf(fp, "%s >%s\n", param_profile->gpufreq_gov, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x20) {
        if ((param_path = get_cpu_param_path("min_sample_time")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->min_sample_time, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x40) {
        if ((param_path = get_cpu_param_path("timer_rate")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->timer_rate, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x80) {
        if ((param_path = get_cpu_param_path("min_cpus")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->min_cpus, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x100) {
        if ((param_path = get_cpu_param_path("max_cpus")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->max_cpus, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x200) {
        if ((param_path = get_cpu_param_path("offline_delay_ms")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->offline_delay_ms, param_path);
        }
    }
    if (param_profile->valid_param_flag & 0x400) {
        if ((param_path = get_cpu_param_path("task_thres")) != NULL) {
            fprintf(fp, "%d >%s\n", param_profile->task_thres, param_path);
        }
    }
    fclose(fp);
    return true;
}

KeyedVector<String8, UserDefinedPowerProfile *>* UserDefinedPowerProfile::userProfileList = new KeyedVector<String8, UserDefinedPowerProfile *>();

bool UserDefinedPowerProfile::addUserPowerProfile(cJSON *configData, bool strict)
{
    String8 profileName;
    cJSON *pData = cJSON_GetObjectItem(configData, "profile_name");
    if (pData && pData->type == cJSON_String && strlen(pData->valuestring) > 0) {
        profileName = String8(pData->valuestring);
    } else {
        return false;
    }

    if (strict && userProfileList->indexOfKey(profileName) >= 0) {
        fprintf(stderr, "profile %s already exist, fail add in strict mode", profileName.string());
        return false;
    }

    struct power_tunable_params *tp;
    cJSON *pProfile = cJSON_GetObjectItem(configData, "tunable_params");
    if (pProfile == NULL) {
       fprintf(stderr, "tunable_params is not present, fail add the profile");
       return false;
    }
    pData = cJSON_GetObjectItem(pProfile, "valid_param_flag");
    if (pData == NULL || pData->type != cJSON_String) {
       fprintf(stderr, "valid_param_flag is not present or not valid, fail add the profile");
       return false;
    }

    tp = (struct power_tunable_params*)calloc(1, sizeof(*tp));
    tp->valid_param_flag = atoi(pData->valuestring);

    pData = cJSON_GetObjectItem(pProfile, "cpufreq_min");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->cpufreq_min = atoi(pData->valuestring);
    }
    pData = cJSON_GetObjectItem(pProfile, "cpufreq_max");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->cpufreq_max = atoi(pData->valuestring);
    }
    pData = cJSON_GetObjectItem(pProfile, "cpufreq_gov");
    if (pData && pData->type == cJSON_String && strlen(pData->valuestring) > 0) {
        strncpy(tp->cpufreq_gov, pData->valuestring, CPUFREQ_NAME_LEN-1);
    }
    pData = cJSON_GetObjectItem(pProfile, "gpufreq_max");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->gpufreq_max = atoi(pData->valuestring);
    }
    pData = cJSON_GetObjectItem(pProfile, "gpufreq_gov");
    if (pData && pData->type == cJSON_String && strlen(pData->valuestring) > 0) {
        strncpy(tp->gpufreq_gov, pData->valuestring, CPUFREQ_NAME_LEN-1);
    }
    pData = cJSON_GetObjectItem(pProfile, "min_sample_time");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->min_sample_time = atoi(pData->valuestring);
    }
    pData = cJSON_GetObjectItem(pProfile, "timer_rate");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->timer_rate = atoi(pData->valuestring);
    }
    pData = cJSON_GetObjectItem(pProfile, "min_cpus");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->min_cpus = atoi(pData->valuestring);
    }
    pData = cJSON_GetObjectItem(pProfile, "max_cpus");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->max_cpus = atoi(pData->valuestring);
    }
    pData = cJSON_GetObjectItem(pProfile, "offline_delay_ms");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->offline_delay_ms = atoi(pData->valuestring);
    }
    pData = cJSON_GetObjectItem(pProfile, "task_thres");
    if (pData && pData->type == cJSON_String && atoi(pData->valuestring) != 0) {
        tp->task_thres = atoi(pData->valuestring);
    }

    UserDefinedPowerProfile *profile = new UserDefinedPowerProfile(profileName, tp);
    if (userProfileList->indexOfKey(profileName) >= 0) {
        struct stat st;
        String8 fullPath = String8(USER_PROFILE_PATH) + profileName;
        if (stat(fullPath.string(), &st) == 0) {
           remove(fullPath.string());
        }
        userProfileList->removeItem(profileName);
    }
    addUserPowerProfile(profile);
    return true;
}

void UserDefinedPowerProfile::addUserPowerProfile(UserDefinedPowerProfile *profile)
{
    userProfileList->add(profile->getProfileName(), profile);
}

UserDefinedPowerProfile * UserDefinedPowerProfile::checkUserPowerProfile(String8 profileName)
{
    if (userProfileList->indexOfKey(profileName) >= 0) {
        struct stat st;
        String8 fullPath = String8(USER_PROFILE_PATH) + profileName;
        UserDefinedPowerProfile *profile = userProfileList->valueFor(profileName);
        if (stat(fullPath.string(), &st) == 0 || profile->writeToFile(fullPath)) {
            return profile;
        }
    }
    return NULL;
}

void UserDefinedPowerProfile::removeAllPowerProfile()
{
    for (int i = userProfileList->size(); i-- > 0;) {
        userProfileList->removeItemsAt(i);
    }
}