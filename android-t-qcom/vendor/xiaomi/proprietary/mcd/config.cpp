#define LOG_TAG "octvm"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <cutils/properties.h>

#include <binder/BinderService.h>
#include <binder/IServiceManager.h>
#include <utils/KeyedVector.h>
#include <utils/String16.h>
#include <utils/String8.h>

#include "octvm.h"
#include "anonymous_id.h"
#include "IDeviceIdentifiersPolicyService.h"
#include "utils/cJSON.h"
#include "octvm/OctVmService.h"
#include "octvm/PowerStateService.h"

extern "C" {
#include "mc_utils.h"
#include "memcontrol_priv.h"
}
void get_serial_number(void);

using namespace android;

////////////////////////////////////////////////////////////////////////////////////////////////////
// OCTVM Global Parameters, default CGROUP/PowerMode initialization
////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned int *get_tokenized_data(const char *buf, int *num_tokens)
{
    int ntokens, i;
    const char *cp;
    unsigned int *tokenized_data;

    cp = buf;
    ntokens = 1;
    while ((cp = strpbrk(cp + 1, " :")))
        ntokens++;

    if (!(ntokens & 0x1))
        return NULL;

    i = 0;
    cp = buf;
    tokenized_data = (unsigned int*)calloc(ntokens, sizeof(unsigned int));
    while (i < ntokens) {
        if (sscanf(cp, "%u", &tokenized_data[i++]) != 1)
            goto err_tokenizer;

        cp = strpbrk(cp, " :");
        if (!cp)
            break;
        cp++;
    }

    if (i != ntokens)
        goto err_tokenizer;

    *num_tokens = ntokens;
    return tokenized_data;

err_tokenizer:
    free(tokenized_data);
    return NULL;
}

/* get config value by total ram size */
static uint32_t get_target_config(const char *config_data)
{
    if (config_data == NULL) {
        fprintf(stderr, "%s:config_data is null\n", __func__);
        return 0;
    }

    int nconfig = 0;
    unsigned int *configs = NULL, target_config = 0;
    configs = get_tokenized_data(config_data, &nconfig);
    if (configs == NULL) {
        fprintf(stderr, "config not valid or get_tokenized_data error");
        return 0;
    }

    int i = 0;
    while (i < nconfig && (i+1) < nconfig) {
        if (gTotalMemoryKB/1024 <= configs[i+1]) {
            break;
        }
        i += 2;
    }
    target_config = configs[i];

    free(configs);
    return target_config;
}

static void parse_memory_config(cJSON *pGroup)
{
    cJSON *pItem;
    if (pGroup != NULL) {
        if (gVMMemoryParams == NULL) {
            gVMMemoryParams = (struct MemoryParams *)calloc(1, sizeof(*gVMMemoryParams));
        }
        pItem = cJSON_GetObjectItem(pGroup, "zram_device_num");
        if (pItem && pItem->type == cJSON_Number) {
            gVMMemoryParams->zram_device_num = pItem->valueint;
        }
        pItem = cJSON_GetObjectItem(pGroup, "zram_size_MB");
        if (pItem && pItem->type == cJSON_String) {
            gVMMemoryParams->zram_total_size = get_target_config(pItem->valuestring);
        }
        pItem = cJSON_GetObjectItem(pGroup, "global_swappiness");
        if (pItem && pItem->type == cJSON_Number) {
            gVMMemoryParams->global_swappiness = pItem->valueint;
        }
        pItem = cJSON_GetObjectItem(pGroup, "more_memory_swappiness");
        if (pItem && pItem->type == cJSON_Number) {
            gVMMemoryParams->more_memory_swappiness = pItem->valueint;
        }
    }
}

static void parse_power_config(cJSON *pGroup)
{
    cJSON *pItem;
    if (pGroup != NULL) {
        if (gVMPowerParams == NULL) {
            gVMPowerParams = (struct PowerSaveParams *)calloc(1, sizeof(struct PowerSaveParams));
        }
        pItem = cJSON_GetObjectItem(pGroup, "defAutoSave");
        if (pItem != NULL && pItem->type == cJSON_Number) {
            gVMPowerParams->default_autosave = pItem->valueint;
        }
        pItem = cJSON_GetObjectItem(pGroup, "windowLength");
        if (pItem != NULL && pItem->type == cJSON_Number) {
            gVMPowerParams->default_params.windowLength = pItem->valueint;
        }
        pItem = cJSON_GetObjectItem(pGroup, "forceIdleOffPct");
        if (pItem != NULL && pItem->type == cJSON_Number) {
            gVMPowerParams->default_params.forceIdleOffPct = pItem->valueint;
        }
        pItem = cJSON_GetObjectItem(pGroup, "forceBusyOffPct");
        if (pItem != NULL && pItem->type == cJSON_Number) {
            gVMPowerParams->default_params.forceBusyOffPct = pItem->valueint;
        }
    }
}

static void parse_cgroups_config(cJSON *pGroup)
{
    cJSON *pItem, *pData;
    if (pGroup != NULL) {
        int num_cgroups = cJSON_GetArraySize(pGroup);
        for (int i = 0; i < num_cgroups; i++) {
            char *groupname = NULL;
            struct task_cgroup *nCGroup = NULL;
            pItem = cJSON_GetArrayItem(pGroup, i);
            pData = cJSON_GetObjectItem(pItem, "groupname");
            if (pData && pData->type == cJSON_String && strlen(pData->valuestring) > 0) {
                groupname = pData->valuestring;
            } else {
                fprintf(stderr, "groupname should not empty, ignore index %d", i);
                continue;
            }
            //check if already in cgroup list
            nCGroup = OctVmService::getSystemCGroup(String8(groupname));
            if (nCGroup == NULL) {
                nCGroup = (struct task_cgroup*)calloc(1, sizeof(*nCGroup)); //set all member to 0
                strcpy(nCGroup->groupname, groupname);
                nCGroup->groupname[MAX_GROUP_NAME_LEN-1] = '\0';
                OctVmService::addSystemCGroup(nCGroup);
            } else {
                printf("override for group %s\n", groupname);
            }
            pData = cJSON_GetObjectItem(pItem, "priority");
            if (pData && pData->type == cJSON_Number) {
                nCGroup->priority = pData->valueint;
            }
            pData = cJSON_GetObjectItem(pItem, "swappiness");
            if (pData && pData->type == cJSON_Number) {
                nCGroup->swappiness = pData->valueint;
            }
            pData = cJSON_GetObjectItem(pItem, "move_charge_at_immigrate");
            if (pData && pData->type == cJSON_Number) {
                nCGroup->move_charge_at_immigrate = pData->valueint;
            }
            pData = cJSON_GetObjectItem(pItem, "limit_in_mega");
            if (pData && pData->type == cJSON_String) {
                nCGroup->mem_limit_in_mega = get_target_config(pData->valuestring);
            }
            pData = cJSON_GetObjectItem(pItem, "soft_limit_in_mega");
            if (pData && pData->type == cJSON_String) {
                nCGroup->mem_soft_limit_in_mega = get_target_config(pData->valuestring);
            }
            pData = cJSON_GetObjectItem(pItem, "def_tasks");
            if (pData && pData->type == cJSON_Array) {
                task_ilist head = NULL, p = NULL;
                int task_num = cJSON_GetArraySize(pData);
                for (int i = 0; i < task_num; i++){
                    char *uid_name = NULL, *task_name = NULL;
                    cJSON *taskItem = cJSON_GetArrayItem(pData, i);
                    cJSON *taskData = cJSON_GetObjectItem(taskItem, "uid");
                    if (taskData && taskData->type == cJSON_String) {
                        uid_name = taskData->valuestring;
                    }
                    taskData = cJSON_GetObjectItem(taskItem, "name");
                    if (taskData && taskData->type == cJSON_String) {
                        task_name = taskData->valuestring;
                    }
                    //in every item, uid and name should not be null
                    if (uid_name == NULL || task_name == NULL) continue;

                    //construct a new task item
                    task_ilist nlist = (task_ilist)calloc(1, sizeof(*nlist));
                    strcpy(nlist->uid_name, uid_name);
                    nlist->uid_name[MAX_UID_NAME_LEN-1] = '\0';
                    strcpy(nlist->task_name, task_name);
                    if (strlen(task_name) > 1 && (task_name[strlen(task_name)-1] == '*')) {
                        nlist->task_name[strlen(task_name)-1] = '\0';//del the end '*'
                    }
                    nlist->task_name[MAX_TASK_NAME_LEN-1] = '\0';
                    if (strcmp(uid_name, "*") != 0) {
                        nlist->check_flag |= UID_CHECK;
                    }
                    if (strcmp(task_name, "*") != 0) {
                        if (task_name[strlen(task_name)-1] == '*') {
                            nlist->check_flag |= TASKNAME_CONTAIN;
                        } else {
                            nlist->check_flag |= TASKNAME_EXACT;
                        }
                    }
                    //put into the task item list
                    if (head == NULL) {
                        head = nlist;
                        p = head;
                    } else {
                        p->next = nlist;
                        p = p->next;
                    }
                }
                if (nCGroup->def_tasks != NULL) {
                    p->next = nCGroup->def_tasks;
                    nCGroup->def_tasks = head;
                } else {
                    nCGroup->def_tasks = head;
                }
            }
        }
    }
}

static void parse_powermode_config(cJSON *pGroup)
{
    cJSON *pItem, *pData;
    if (pGroup != NULL) {
        int num_modes = cJSON_GetArraySize(pGroup);
        for (int i = 0; i < num_modes; i++) {
            char * mode_name = NULL;
            struct power_mode *nMode = NULL;
            int32_t mode_id = 0, type_id = 0, autosave = 0;
            pItem = cJSON_GetArrayItem(pGroup, i);
            pData = cJSON_GetObjectItem(pItem, "mode_id");
            if (pData && pData->type == cJSON_String && strlen(pData->valuestring) > 0) {
                mode_id = atoi(pData->valuestring);
            } else {
                fprintf(stderr,"mode_id should not empty\n");
                continue;
            }
            pData = cJSON_GetObjectItem(pItem, "mode_name");
            if (pData && pData->type == cJSON_String && strlen(pData->valuestring) > 0) {
                mode_name = pData->valuestring;
            }
            //check if already added
            nMode = PowerStateService::getSystemPowerMode(mode_id);
            if (nMode == NULL && mode_name == NULL) {
                fprintf(stderr,"mode_name should not empty\n");
                continue;
            } else if (nMode != NULL) {
                printf("override for power mode %s\n", nMode->mode_property);
                if (mode_name != NULL) {
                    printf("mode_name override will ignored");
                }
            } else {
                int name_length = strlen(mode_name) + 1;
                nMode = (struct power_mode *)calloc(1, sizeof(struct power_mode) + name_length);
                nMode->len = name_length;
                nMode->hdr_size = sizeof(struct power_mode);
                nMode->mode_id = mode_id;
                strncpy(nMode->mode_property, mode_name, name_length);
                nMode->mode_property[name_length - 1] = '\0';
                PowerStateService::addSystemDefaultPowerMode(nMode);
            }
            pData = cJSON_GetObjectItem(pItem, "mode_type");
            if (pData && pData->type == cJSON_String && strlen(pData->valuestring) > 0) {
                nMode->type_id = atoi(pData->valuestring);
            }
            nMode->autosave = gVMPowerParams->default_autosave;
            pData = cJSON_GetObjectItem(pItem, "autosave");
            if (pData && pData->type == cJSON_Number) {
                nMode->autosave = pData->valueint;
            }
        }
    }
}

static cJSON *find_override_config(cJSON *config_data, const char *config_item)
{
    cJSON *pOverride = NULL;
    String8 itemKey;
    itemKey.appendFormat("override_%s", config_item);
    cJSON *pConfig = cJSON_GetObjectItem(config_data, itemKey.string());
    if (pConfig != NULL) {
        int override_num = cJSON_GetArraySize(pConfig);
        for (int i = 0; i < override_num; i++) {
            bool find_override = false;
            cJSON *pItem = cJSON_GetArrayItem(pConfig, i);
            cJSON *pData = cJSON_GetObjectItem(pItem, "model");
            if (pData != NULL) {
                int override_model_num = cJSON_GetArraySize(pData);
                for (int j = 0; j < override_model_num; j++) {
                    char *oModel = cJSON_GetArrayItem(pData,j)->valuestring;
                    if (oModel != NULL && strcmp(gModel, oModel) == 0) {
                        printf("find %s config override for product:[%s]\n", config_item, gModel);
                        pOverride = cJSON_GetObjectItem(pItem, config_item);
                        find_override = true;
                        break;
                    }
                }
            }
            if (find_override) break;
        }
    }
    return pOverride;
}

bool global_config_init()
{
    get_serial_number();

    property_get("ro.product.model", gModel, "");
    printf("model:%s\n", gModel);
    if (!gModel[0]) {
        fprintf(stderr, "*** CANNOT GET THE PRODUCT MODEL\n\n");
        return false;
    }
    property_get("ro.product.device", gDevice, "");
    printf("device:%s\n", gDevice);
    if (!gDevice[0]) {
        fprintf(stderr, "*** CANNOT GET THE PRODUCT MODEL\n\n");
        return false;
    }
    property_get("ro.product.mod_device", gModDevice, "");
    printf("mod_device:%s\n", gModDevice);
    if (!gModDevice[0]) {
        fprintf(stderr, "*** CANNOT GET THE PRODUCT MODEL\n\n");
    }
    gTotalMemoryKB = get_total_memory_KB();
    printf("total memory:%ld\n", gTotalMemoryKB);
    if (gTotalMemoryKB == 0) {
        fprintf(stderr, "*** CANNOT GET DEVICE MEMORY SIZE\n\n");
        return false;
    }

    char config_file_path[PROPERTY_VALUE_MAX];
    memset(config_file_path, 0, PROPERTY_VALUE_MAX);
    property_get("persist.sys.mcd_config_file", config_file_path, "");
    if (!config_file_path[0]) {
        // 添加默认值的处理
        strcpy(config_file_path, "/system/etc/mcd_default.conf");
        fprintf(stderr, "*** NO MCD CONFIG FILE DEFINED (persist.sys.mcd_config_file) so use default\n\n");
    }
    //load config data
    FILE *fp = fopen(config_file_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "*** MCD CONFIG FILE NOT FOUND (%s)\n\n", config_file_path);
        return false;
    }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *config_data = (char *)malloc(len+1);
    memset(config_data, 0, len+1);
    fread(config_data, 1, len, fp);
    fclose(fp);

    cJSON *pRoot, *pItem;
    pRoot = cJSON_Parse(config_data);
    if (!pRoot) {
        fprintf(stderr, "JSON format error: before [%s]", cJSON_GetErrorPtr());
        return false;
    }

    //get octvm global config
    pItem = cJSON_GetObjectItem(pRoot, "memory_opt");
    parse_memory_config(pItem);
    pItem = find_override_config(pRoot, "memory_opt");
    if (pItem != NULL) {
        parse_memory_config(pItem);
    }
    pItem = cJSON_GetObjectItem(pRoot, "cgroups");
    parse_cgroups_config(pItem);
    pItem = find_override_config(pRoot, "cgroups");
    if (pItem != NULL) {
        parse_cgroups_config(pItem);
    }

    pItem = cJSON_GetObjectItem(pRoot, "power_save");
    parse_power_config(pItem);
    pItem = find_override_config(pRoot, "power_save");
    if (pItem != NULL) {
        parse_power_config(pItem);
    }
    pItem = cJSON_GetObjectItem(pRoot, "power_modes");
    parse_powermode_config(pItem);
    pItem = find_override_config(pRoot, "power_modes");
    if (pItem != NULL) {
        parse_powermode_config(pItem);
    }

    cJSON_Delete(pRoot);
    free(config_data);
    return true;
}

void get_serial_number()
{
    memset(serialno, 0, sizeof(serialno));
#if (PLATFORM_SDK_VERSION >= 26)
    sp<IBinder> service = defaultServiceManager()->checkService(String16("device_identifiers"));
    if (service != NULL) {
        sp<IBinder> binder = defaultServiceManager()->getService(String16("device_identifiers"));
        sp<IDeviceIdentifiersPolicyService> idips = interface_cast<IDeviceIdentifiersPolicyService>(binder);
        String16 serial = idips->getSerial();
        String8 serial_name8 = String8(serial);
        const char *ret = serial_name8.string();
        if (ret != NULL) {
            strncpy(serialno, ret, sizeof(serialno));
            serialno[sizeof(serialno)-1] = 0;
        }
    }
#else
    property_get(SERIALNO_PROP, serialno, "");
#endif
}
