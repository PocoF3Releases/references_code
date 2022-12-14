/*
 * Copyright (c) 2021, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#include <vendor/qti/hardware/ListenSoundModel/1.0/IListenSoundModel.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <log/log.h>
#include <cutils/properties.h>
#include "inc/lsm_client_wrapper.h"

using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::hidl_memory;
using android::hardware::Return;
using android::hardware::hidl_vec;
using vendor::qti::hardware::ListenSoundModel::V1_0::IListenSoundModel;
using android::sp;

bool lsm_server_died = false;
android::sp<IListenSoundModel> lsm_client = NULL;
sp<server_death_notifier> Server_death_notifier = NULL;
sp<IAllocator> ashmemAllocator = NULL;
char vendor_soc_model[PROPERTY_VALUE_MAX] = {'\0'};
bool is_soc_model_queried = false;

void server_death_notifier::serviceDied(uint64_t cookie __unused,
    const android::wp<::android::hidl::base::V1_0::IBase>& who __unused)
{
    ALOGE("%s: LSM Service died", __func__);
    lsm_server_died = true;
    /*
     * We exit the client process here, so that it also can restart
     * leading to a fresh start on both the sides.
     */
    _exit(1);
}

android::sp<IListenSoundModel> getLsmServer()
{
    if (lsm_client == NULL) {
        lsm_client = IListenSoundModel::getService();
        if (lsm_client == NULL) {
            ALOGE("%s: LSM service not initialized", __func__);
            goto exit;
        }
        if (Server_death_notifier == NULL) {
            Server_death_notifier = new server_death_notifier();
            lsm_client->linkToDeath(Server_death_notifier, 0);
            ALOGD("%s: LSM client registered for server death notification",
                __func__);
        }
        ashmemAllocator = IAllocator::getService("ashmem");
    }
exit:
    return lsm_client ;
}

bool doesSocSupportHIDL(void)
{
    if (!is_soc_model_queried) {
        if (property_get("ro.soc.model", vendor_soc_model, "") <= 0)
           ALOGE("Unable to get the vendor soc model name");
        else
           ALOGD("soc model obtained:%s", vendor_soc_model);
        is_soc_model_queried = true;
    }
    /* Add all the non supported targets for LSM HIDL Interface here */
    if (!strncmp(vendor_soc_model, "SM8350", strlen("SM8350")) ||
        !strncmp(vendor_soc_model, "SM7350", strlen("SM7350")) ||
        !strncmp(vendor_soc_model, "SM7325", strlen("SM7325")) ||
        !strncmp(vendor_soc_model, "SM4350", strlen("SM4350")))
        return false;
    else
        return true;
}

bool isLsmHidlClientSupported(void)
{
    /* If the vendor soc model doesn't support HIDL interface, return false from here */
    if (!doesSocSupportHIDL())
        return false;

    if (lsm_client == NULL)
        lsm_client = getLsmServer();
    return lsm_client == NULL ? false : true;
}

int32_t mapFromHidlMemory(void* out_data, int32_t size, const hidl_memory& mem)
{
    void *data = NULL;

    sp<IMemory> memory = mapMemory(mem);
    if (memory == NULL) {
        ALOGE("%s: Could not map HIDL mem to IMemory", __func__);
        return -EINVAL;
    }
    if (memory->getSize() != mem.size()) {
        ALOGE("%s: Size mismatch in memory mapping", __func__);
        return -EINVAL;
    }
    data = memory->getPointer();
    if (data == NULL) {
        ALOGE("%s: Could not get memory pointer", __func__);
        return -EINVAL;
    }

    memory->update();
    memcpy(out_data, data, size);
    memory->commit();

    return 0;
}

int32_t mapToHidlMemory(void* inp_data, int32_t size, const hidl_memory& mem)
{
    void *data = NULL;

    sp<IMemory> memory = mapMemory(mem);
    if (memory == NULL) {
        ALOGE("%s: Could not map HIDL mem to IMemory", __func__);
        return -EINVAL;
    }
    if (memory->getSize() != mem.size()) {
        ALOGE("%s: Size mismatch in memory mapping", __func__);
        return -EINVAL;
    }
    data = memory->getPointer();
    if (data == NULL) {
        ALOGE("%s: Could not get memory pointer", __func__);
        return -EINVAL;
    }

    memory->update();
    memcpy(data, inp_data, size);
    memory->commit();

    return 0;
}

int32_t getHidlMemory(void *inp_data, int32_t size, hidl_memory& hidl_mem)
{
    int32_t status = 0;

    if (ashmemAllocator == NULL) {
        ALOGE("%s: Memory allocator is invalid", __func__);
        return -ENOMEM;
    }

    ashmemAllocator->allocate(size, [&](bool success, const hidl_memory& mem) {
        if (!success) {
            ALOGE("%s: Memory allocation failed", __func__);
            status = -ENOMEM;
            return;
        }
        hidl_mem = mem;
        status = mapToHidlMemory(inp_data, size, mem);
        if (status < 0) {
            return;
        }
    });

    return status;
}

listen_status_enum lsm_findKeywordEndPosition(listen_model_type *pKeywordModel,
    keywordId_t keywordId, listen_user_recording *pUserRecording,
    uint32_t *pKendPosition)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pKeywordModel || !pUserRecording || !pKendPosition) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);

        hidl_memory model_mem, rec_mem;
        ret = getHidlMemory(pKeywordModel->data, pKeywordModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        ret = getHidlMemory(pUserRecording->data,
            pUserRecording->n_samples * sizeof(int16_t), rec_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_findKeywordEndPosition(model_mem, pKeywordModel->size,
            keyId, rec_mem, pUserRecording->n_samples * sizeof(int16_t),
            [&](uint32_t status_, uint32_t kEndPositionRet) {
                if (!status_)
                    *pKendPosition = kEndPositionRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_verifyUserRecording(listen_model_type *pKeywordModel,
    keywordId_t keywordId, listen_epd_params *pEpdParameter,
    listen_user_recording *pUserRecording, int16_t *pConfidenceLevel)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    /* pEpdParameter can be passed as NULL to take default values */
    if (!pKeywordModel || !pUserRecording || !pConfidenceLevel) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);
        hidl_vec<ListenEpdParams> epd_params_hidl;

        if (pEpdParameter) {
            epd_params_hidl.resize(1);
            memcpy(epd_params_hidl.data(), pEpdParameter, sizeof(ListenEpdParams));
        }

        hidl_memory model_mem, rec_mem;
        ret = getHidlMemory(pKeywordModel->data, pKeywordModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        ret = getHidlMemory(pUserRecording->data,
            pUserRecording->n_samples * sizeof(int16_t), rec_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_verifyUserRecording(model_mem, pKeywordModel->size,
            keyId, epd_params_hidl,
            rec_mem, pUserRecording->n_samples * sizeof(int16_t),
            [&](uint32_t status_, int16_t confidenceLevelRet) {
                if (!status_)
                    *pConfidenceLevel = confidenceLevelRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_checkUserRecording(listen_language_model_type *pLanguageModel,
    listen_epd_params *pEpdParameter,
    listen_user_recording *pUserRecording, float *pOutSnr,
    uint32_t maxPhonemeLength)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    /* pEpdParameter can be passed as NULL to take default values */
    if (!pLanguageModel || !pUserRecording || !pOutSnr) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<ListenEpdParams> epd_params_hidl;
        if (pEpdParameter) {
            epd_params_hidl.resize(1);
            memcpy(epd_params_hidl.data(), pEpdParameter, sizeof(ListenEpdParams));
        }

        hidl_memory lang_model_mem, rec_mem;
        ret = getHidlMemory(pLanguageModel->data, pLanguageModel->size, lang_model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        ret = getHidlMemory(pUserRecording->data,
            pUserRecording->n_samples * sizeof(int16_t), rec_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_checkUserRecording(lang_model_mem, pLanguageModel->size,
            epd_params_hidl, rec_mem,
            pUserRecording->n_samples * sizeof(int16_t), maxPhonemeLength,
            [&](uint32_t status_, float outSnrRet) {
                if (!status_)
                    *pOutSnr = outSnrRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_checkRecordingsQuality(
    listen_language_model_type *pLanguageModel,
    listen_epd_params *pEpdParameter, uint32_t numUserRecording,
    listen_user_recording *pUserRecordings[], float *pOutSnr)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    /* pEpdParameter can be passed as NULL to take default values */
    if (!pLanguageModel || !pUserRecordings || !pOutSnr) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<ListenEpdParams> epd_params_hidl;
        if (pEpdParameter) {
            epd_params_hidl.resize(1);
            memcpy(epd_params_hidl.data(), pEpdParameter,
                sizeof(ListenEpdParams));
        }

        hidl_memory lang_model_mem;
        ret = getHidlMemory(pLanguageModel->data, pLanguageModel->size, lang_model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        hidl_vec<hidl_memory> recs_mem;
        recs_mem.resize(numUserRecording);
        hidl_vec<int32_t> recs_size;
        recs_size.resize(numUserRecording);
        for (uint32_t i = 0; i < numUserRecording; i++) {
            recs_size[i] = pUserRecordings[i]->n_samples * sizeof(int16_t);
            ret = getHidlMemory(pUserRecordings[i]->data,
                recs_size[i], recs_mem[i]);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        lsm_client->ipc_lsm_checkRecordingsQuality(lang_model_mem, pLanguageModel->size,
            epd_params_hidl, numUserRecording,
            recs_mem, recs_size,
            [&](uint32_t status_, float outSnrRet) {
                if (!status_)
                    *pOutSnr = outSnrRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_tuneUserDefinedKeywordModelThreshold(
    listen_model_type *pUserDefinedKeyword,
    keywordId_t keywordId, listen_user_recording *pUserRecording,
    listen_model_type *pOutputUserDefinedKeyword)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0, model_size = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pUserDefinedKeyword || !pUserRecording || !pOutputUserDefinedKeyword) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);

        hidl_memory model_mem, rec_mem;
        if (pUserDefinedKeyword) {
            model_size = pUserDefinedKeyword->size;
            ret = getHidlMemory(pUserDefinedKeyword->data, model_size, model_mem);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        ret = getHidlMemory(pUserRecording->data,
            pUserRecording->n_samples * sizeof(int16_t), rec_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_tuneUserDefinedKeywordModelThreshold(model_mem,
            model_size, keyId, rec_mem,
            pUserRecording->n_samples * sizeof(int16_t),
            [&](uint32_t status_,
            hidl_memory retModelMem, int32_t outModelSize) {
                int32_t ret = 0;
                if (!status_) {
                    pOutputUserDefinedKeyword->size = outModelSize;
                    ret = mapFromHidlMemory(pOutputUserDefinedKeyword->data,
                        outModelSize, retModelMem);
                    if (ret < 0)
                        status_ = kFailed;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getUserDefinedKeywordSize(
    listen_model_type *pUserDefinedKeyword,
    keywordId_t keywordId, userId_t userId,
    listen_epd_params *pEpdParameter, uint32_t numUserRecording,
    listen_user_recording *pUserRecordings[],
    listen_language_model_type *pLanguageModel, uint32_t *pOutputSize)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0, model_size = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    /* pEpdParameter can be passed as NULL to take default values */
    if (!pUserRecordings || !pLanguageModel) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);
        std::string uId(userId);

        hidl_vec<ListenEpdParams> epd_params_hidl;
        if (pEpdParameter) {
            epd_params_hidl.resize(1);
            memcpy(epd_params_hidl.data(), pEpdParameter, sizeof(ListenEpdParams));
        }

        hidl_memory model_mem;
        if (pUserDefinedKeyword) {
            model_size = pUserDefinedKeyword->size;
            ret = getHidlMemory(pUserDefinedKeyword->data, model_size, model_mem);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        hidl_memory lang_model_mem;
        ret = getHidlMemory(pLanguageModel->data, pLanguageModel->size, lang_model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        hidl_vec<hidl_memory> recs_mem;
        recs_mem.resize(numUserRecording);
        hidl_vec<int32_t> recs_size;
        recs_size.resize(numUserRecording);
        for (uint32_t i = 0; i < numUserRecording; i++) {
            recs_size[i] = pUserRecordings[i]->n_samples * sizeof(int16_t);
            ret = getHidlMemory(pUserRecordings[i]->data,
                recs_size[i], recs_mem[i]);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        lsm_client->ipc_lsm_getUserDefinedKeywordSize(model_mem, model_size,
            keyId, uId,
            epd_params_hidl, numUserRecording,
            recs_mem, recs_size, lang_model_mem,
            pLanguageModel->size,
            [&](uint32_t status_, uint32_t outputSizeRet) {
                if (!status_)
                    *pOutputSize = outputSizeRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getUserDefinedKeywordApproxSize(
    keywordId_t keywordId, userId_t userId,
    listen_language_model_type *pLanguageModel, uint32_t *pOutputSize,
    uint32_t maxPhonemeLength)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pOutputSize || !pLanguageModel) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);
        std::string uId(userId);

        hidl_memory lang_model_mem;
        ret = getHidlMemory(pLanguageModel->data, pLanguageModel->size, lang_model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getUserDefinedKeywordApproxSize(keyId, uId,
            lang_model_mem, pLanguageModel->size,
            maxPhonemeLength,
            [&](uint32_t status_, uint32_t outputSizeRet) {
                if (!status_)
                    *pOutputSize = outputSizeRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_createUserDefinedKeywordModel(
    listen_model_type *pUserDefinedKeyword,
    keywordId_t keywordId, userId_t userId,
    listen_epd_params *pEpdParameter, uint32_t numUserRecording,
    listen_user_recording *pUserRecordings[],
    listen_language_model_type *pLanguageModel,
    listen_model_type *pOutputUserDefinedKeyword, int16_t *pMatchingScore)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0, model_size = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    /* pEpdParameter can be passed as NULL to take default values */
    if (!pUserRecordings || !pLanguageModel ||
        !pOutputUserDefinedKeyword || !pMatchingScore ||
        (pOutputUserDefinedKeyword->size == 0)) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);
        std::string uId(userId);

        hidl_vec<ListenEpdParams> epd_params_hidl;
        if (pEpdParameter) {
            epd_params_hidl.resize(1);
            memcpy(epd_params_hidl.data(), pEpdParameter, sizeof(ListenEpdParams));
        }

        hidl_memory model_mem;
        if (pUserDefinedKeyword) {
            model_size = pUserDefinedKeyword->size;
            ret = getHidlMemory(pUserDefinedKeyword->data, model_size, model_mem);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        hidl_memory lang_model_mem;
        ret = getHidlMemory(pLanguageModel->data, pLanguageModel->size, lang_model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        hidl_vec<hidl_memory> recs_mem;
        recs_mem.resize(numUserRecording);
        hidl_vec<int32_t> recs_size;
        recs_size.resize(numUserRecording);
        for (uint32_t i = 0; i < numUserRecording; i++) {
            recs_size[i] = pUserRecordings[i]->n_samples * sizeof(int16_t);
            ret = getHidlMemory(pUserRecordings[i]->data,
                recs_size[i], recs_mem[i]);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        lsm_client->ipc_lsm_createUserDefinedKeywordModel(model_mem, model_size,
            keyId, uId, epd_params_hidl, numUserRecording,
            recs_mem, recs_size, lang_model_mem, pLanguageModel->size,
            pOutputUserDefinedKeyword->size,
            [&](uint32_t status_,
            hidl_memory retModelMem, int16_t matchingScore) {
                int32_t ret = 0;
                if (!status_) {
                    *pMatchingScore = matchingScore;
                    ret = mapFromHidlMemory(pOutputUserDefinedKeyword->data,
                        pOutputUserDefinedKeyword->size, retModelMem);
                    if (ret < 0)
                        status_ = kFailed;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getStrippedUserKeywordModelSize(
    listen_model_type *pModel, uint32_t *nStrippedModelSize)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pModel || !nStrippedModelSize) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pModel->data, pModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getStrippedUserKeywordModelSize(model_mem, pModel->size,
            [&](uint32_t status_, uint32_t sModelSizeRet) {
                if (!status_)
                   *nStrippedModelSize = sModelSizeRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_stripUserKeywordModel(listen_model_type *pModel,
    listen_model_type *pStrippedModel)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pModel || !pStrippedModel) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pModel->data, pModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_stripUserKeywordModel(model_mem, pModel->size,
            [&](uint32_t status_, hidl_memory retModelMem, int32_t outModelSize) {
                int32_t ret = 0;
                if (!status_) {
                    pStrippedModel->size = outModelSize;
                    ret = mapFromHidlMemory(pStrippedModel->data,
                        outModelSize, retModelMem);
                    if (ret < 0)
                        status_ = kFailed;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getUserKeywordModelSize(listen_model_type *pKeywordModel,
    keywordId_t keywordId, userId_t userId, uint32_t *nUserKeywordModelSize)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!nUserKeywordModelSize || !pKeywordModel) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);
        std::string uId(userId);

        hidl_memory model_mem;
        ret = getHidlMemory(pKeywordModel->data, pKeywordModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getUserKeywordModelSize(model_mem, pKeywordModel->size,
            keyId, uId,
            [&](uint32_t status_, uint32_t sizeRet) {
                if (!status_)
                    *nUserKeywordModelSize = sizeRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_createUserKeywordModel(
    listen_model_type *pKeywordModel,
    keywordId_t keywordId, userId_t userId,
    listen_epd_params *pEpdParameter,
    uint32_t numUserRecording,
    listen_user_recording *pUserRecordings[],
    listen_model_type *pUserKeywordModel,
    int16_t *pUserMatchingScore)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pKeywordModel || !pUserRecordings ||
        !pUserKeywordModel || !pUserMatchingScore ||
        (pUserKeywordModel->size == 0))  {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);
        std::string uId(userId);

        hidl_vec<ListenEpdParams> epd_params_hidl;
        if (pEpdParameter) {
            epd_params_hidl.resize(1);
            memcpy(epd_params_hidl.data(), pEpdParameter, sizeof(ListenEpdParams));
        }

        hidl_memory model_mem;
        ret = getHidlMemory(pKeywordModel->data, pKeywordModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        hidl_vec<hidl_memory> recs_mem;
        recs_mem.resize(numUserRecording);
        hidl_vec<int32_t> recs_size;
        recs_size.resize(numUserRecording);
        for (uint32_t i = 0; i < numUserRecording; i++) {
            recs_size.data()[i] = pUserRecordings[i]->n_samples * sizeof(int16_t);
            ret = getHidlMemory(pUserRecordings[i]->data,
                recs_size[i], recs_mem[i]);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        lsm_client->ipc_lsm_createUserKeywordModel(model_mem, pKeywordModel->size,
            keyId, uId, epd_params_hidl, numUserRecording,
            recs_mem, recs_size, pUserKeywordModel->size,
            [&](uint32_t status_,
            hidl_memory retModelMem, int16_t matchingScoreRet) {
                int32_t ret = 0;
                if (!status_) {
                    *pUserMatchingScore = matchingScoreRet;
                    ret = mapFromHidlMemory(pUserKeywordModel->data,
                        pUserKeywordModel->size, retModelMem);
                    if (ret < 0)
                        status_ = kFailed;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getSizeAfterDeleting(listen_model_type *pInputModel,
    keywordId_t keywordId, userId_t userId, uint32_t *nOutputModelSize)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;
    std::string keyId;
    std::string uId;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pInputModel || !nOutputModelSize) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        if (keywordId)
            keyId = keywordId;
        if (userId)
            uId = userId;

        hidl_memory model_mem;
        ret = getHidlMemory(pInputModel->data, pInputModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getSizeAfterDeleting(model_mem, pInputModel->size,
            keyId, uId,
            [&](uint32_t status_, uint32_t outModelSizeRet) {
                if (!status_)
                    *nOutputModelSize = outModelSizeRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_deleteFromModel(listen_model_type *pInputModel,
    keywordId_t keywordId, userId_t userId, listen_model_type *pResultModel)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;
    std::string keyId;
    std::string uId;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pInputModel || !pResultModel || (pResultModel->size == 0)) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        if (keywordId)
            keyId = keywordId;
        if (userId)
            uId = userId;

        hidl_memory model_mem;
        ret = getHidlMemory(pInputModel->data, pInputModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_deleteFromModel(model_mem, pInputModel->size,
            keyId, uId, pResultModel->size,
            [&](uint32_t status_, hidl_memory retModelMem) {
                int32_t ret = 0;
                if (!status_) {
                    ret = mapFromHidlMemory(pResultModel->data,
                        pResultModel->size, retModelMem);
                    if (ret < 0)
                        status_ = kFailed;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}
listen_status_enum lsm_getMergedModelSize(uint16_t numModels,
    listen_model_type *pModels[], uint32_t *nOutputModelSize)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pModels || !nOutputModelSize || numModels == 0) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<hidl_memory> models_mem;
        models_mem.resize(numModels);
        hidl_vec<int32_t> models_size;
        models_size.resize(numModels);
        for (uint32_t i = 0; i < numModels; i++) {
            models_size[i] = pModels[i]->size;
            ret = getHidlMemory(pModels[i]->data,
                models_size[i], models_mem[i]);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        lsm_client->ipc_lsm_getMergedModelSize(numModels, models_mem, models_size,
            [&](uint32_t status_, uint32_t outModelSizeRet) {
                if (!status_)
                    *nOutputModelSize = outModelSizeRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_mergeModels(uint16_t numModels,
    listen_model_type *pModels[], listen_model_type *pMergedModel)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pModels || !pMergedModel || numModels == 0 ||
        pMergedModel->size == 0) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<hidl_memory> models_mem;
        models_mem.resize(numModels);
        hidl_vec<int32_t> models_size;
        models_size.resize(numModels);
        for (uint32_t i = 0; i < numModels; i++) {
            models_size[i] = pModels[i]->size;
            ret = getHidlMemory(pModels[i]->data,
                models_size[i], models_mem[i]);
            if (ret < 0) {
                ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                return status;
            }
        }

        lsm_client->ipc_lsm_mergeModels(numModels, models_mem, models_size,
            pMergedModel->size,
            [&](uint32_t status_, hidl_memory retModelMem) {
                int32_t ret = 0;
                if (!status_) {
                    ret = mapFromHidlMemory(pMergedModel->data,
                        pMergedModel->size, retModelMem);
                    if (ret < 0)
                        status_ = kFailed;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_parseFromBigSoundModel(listen_model_type *pSM3p0Model,
    listen_model_type *p1stStageModel, listen_model_type *p2ndStageKWModel,
    listen_model_type *p2stStageVoPModel, uint16_t *indicator)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pSM3p0Model || !p1stStageModel || !p2ndStageKWModel ||
        !p2stStageVoPModel || !indicator) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pSM3p0Model->data, pSM3p0Model->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_parseFromBigSoundModel(model_mem, pSM3p0Model->size,
            [&](uint32_t status_, hidl_memory ret1stStgModel,
            int32_t ret1stStgModelSize,
            hidl_memory ret2ndStgKwModel, int32_t ret2ndStgKwModelSize,
            hidl_memory ret2ndStgVopModel, int32_t ret2ndStgVopModelSize,
            uint16_t indicatorRet) {
                int32_t ret = 0;
                if (!status_) {
                    p1stStageModel->size = ret1stStgModelSize;
                    ret = mapFromHidlMemory(p1stStageModel->data,
                        ret1stStgModelSize, ret1stStgModel);
                    if (ret < 0) {
                        status_ = kFailed;
                        return;
                    }
                    p2ndStageKWModel->size = ret2ndStgKwModelSize;
                    ret = mapFromHidlMemory(p2ndStageKWModel->data,
                        ret2ndStgKwModelSize, ret2ndStgKwModel);
                    if (ret < 0) {
                        status_ = kFailed;
                        return;
                    }
                    p2stStageVoPModel->size = ret2ndStgVopModelSize;
                    ret = mapFromHidlMemory(p1stStageModel->data,
                        ret2ndStgVopModelSize, ret2ndStgVopModel);
                    if (ret < 0) {
                        status_ = kFailed;
                        return;
                    }
                    *indicator = indicatorRet;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_parseDetectionEventData(listen_model_type *pUserKeywordModel,
    listen_event_payload *pEventPayload,
    listen_detection_event_type *pDetectionEvent)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pUserKeywordModel || !pEventPayload || !pDetectionEvent) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pUserKeywordModel->data, pUserKeywordModel->size,
            model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        hidl_vec<ListenEventPayload> evt_payload_hidl;
        evt_payload_hidl.resize(1);
        evt_payload_hidl.data()->status = (ListenDetectionStatusEnum) pEventPayload->status;
        evt_payload_hidl.data()->data.resize(pEventPayload->size);
        evt_payload_hidl.data()->size = pEventPayload->size;
        memcpy(evt_payload_hidl.data()->data.data(), pEventPayload->data,
            pEventPayload->size);

        lsm_client->ipc_lsm_parseDetectionEventData(model_mem, pUserKeywordModel->size,
            evt_payload_hidl,
            [&](uint32_t status_, hidl_vec<ListenDetectionEventType> ret_evt_hidl) {
                if (!status_) {
                    pDetectionEvent->detection_data_type =
                        (listen_detection_type_enum) ret_evt_hidl.data()->detection_data_type;
                    if (pDetectionEvent->detection_data_type ==
                        kSingleKWDetectionEvent) {
                        strlcpy(pDetectionEvent->event.event_v1.keyword,
                            ret_evt_hidl.data()->event.event_v1().keyword.c_str(),
                            MAX_STRING_LEN);
                        pDetectionEvent->event.event_v1.keywordConfidenceLevel =
                            ret_evt_hidl.data()->event.event_v1().keywordConfidenceLevel;
                        pDetectionEvent->event.event_v1.userConfidenceLevel =
                            ret_evt_hidl.data()->event.event_v1().userConfidenceLevel;
                    } else {
                        strlcpy(pDetectionEvent->event.event_v2.keywordPhrase,
                            ret_evt_hidl.data()->event.event_v2().keywordPhrase.c_str(),
                            MAX_STRING_LEN);
                        strlcpy(pDetectionEvent->event.event_v2.userName,
                            ret_evt_hidl.data()->event.event_v2().userName.c_str(),
                            MAX_STRING_LEN);
                        pDetectionEvent->event.event_v2.highestKeywordConfidenceLevel =
                            ret_evt_hidl.data()->event.event_v2().highestKeywordConfidenceLevel;
                        pDetectionEvent->event.event_v2.highestUserConfidenceLevel =
                            ret_evt_hidl.data()->event.event_v2().highestUserConfidenceLevel;
                        pDetectionEvent->event.event_v2.pairConfidenceLevels.size =
                            ret_evt_hidl.data()->event.event_v2().pairConfidenceLevels.size;
                        memcpy(pDetectionEvent->event.event_v2.pairConfidenceLevels.pConfLevels,
                            ret_evt_hidl.data()->event.event_v2().pairConfidenceLevels.pConfLevels.data(),
                            pDetectionEvent->event.event_v2.pairConfidenceLevels.size);
                    }
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_querySoundModel(listen_model_type *pSoundModel,
    listen_sound_model_info *pListenSoundModelInfo)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pSoundModel || !pListenSoundModelInfo) {
        ALOGE("%s:%d: Invalid input arguments", __func__, __LINE__);
        return status;
    }

    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pSoundModel->data, pSoundModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        Return<void> ret = lsm_client->ipc_lsm_querySoundModel(model_mem, pSoundModel->size,
            [&](uint32_t status_, hidl_vec<ListenSoundModelInfo> retModelInfo) {
                if (!ret.isOk()) {
                   ALOGE("%s: Call to ipc_lsm_querySoundModel() failed", __func__);
                } else if (!status_) {
                    pListenSoundModelInfo->type =
                        (listen_model_enum) retModelInfo.data()->type;
                    pListenSoundModelInfo->version =
                        retModelInfo.data()->version;
                    pListenSoundModelInfo->size =
                        retModelInfo.data()->size;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getSoundModelHeader(listen_model_type *pSoundModel,
    listen_sound_model_header *pListenSoundModelHeader)
{
    listen_status_enum status = kFailed;
    uint16_t num_kw = 0, num_users = 0;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pSoundModel || !pListenSoundModelHeader) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pSoundModel->data, pSoundModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getSoundModelHeader(model_mem, pSoundModel->size,
            [&](uint32_t status_, hidl_vec<ListenSoundModelHeader> ret_hdr_hidl) {
                if (!status_) {
                    pListenSoundModelHeader->numKeywords =
                        ret_hdr_hidl.data()->numKeywords;
                    pListenSoundModelHeader->numUsers =
                        ret_hdr_hidl.data()->numUsers;
                    pListenSoundModelHeader->numActiveUserKeywordPairs =
                        ret_hdr_hidl.data()->numActiveUserKeywordPairs;
                    pListenSoundModelHeader->isStripped =
                        ret_hdr_hidl.data()->isStripped;
                    num_kw = pListenSoundModelHeader->numKeywords;
                    num_users = pListenSoundModelHeader->numUsers;
                    if (ret_hdr_hidl.data()->langPerKw.data()) {
                        pListenSoundModelHeader->langPerKw =
                            (uint16_t*)calloc(1, num_kw * sizeof(uint16_t));
                        if (pListenSoundModelHeader->langPerKw)
                            memcpy(pListenSoundModelHeader->langPerKw,
                                ret_hdr_hidl.data()->langPerKw.data(),
                                num_kw * sizeof(uint16_t));
                    }
                    if (ret_hdr_hidl.data()->numUsersSetPerKw.data()) {
                        pListenSoundModelHeader->numUsersSetPerKw =
                            (uint16_t*)calloc(1, num_kw * sizeof(uint16_t));
                        if (pListenSoundModelHeader->numUsersSetPerKw)
                            memcpy(pListenSoundModelHeader->numUsersSetPerKw,
                                ret_hdr_hidl.data()->numUsersSetPerKw.data(),
                                num_kw * sizeof(uint16_t));
                    }
                    if (ret_hdr_hidl.data()->isUserDefinedKeyword.data()) {
                        pListenSoundModelHeader->isUserDefinedKeyword =
                            (bool*)calloc(1, num_kw * sizeof(bool));
                        if (pListenSoundModelHeader->isUserDefinedKeyword)
                            memcpy(pListenSoundModelHeader->isUserDefinedKeyword,
                                ret_hdr_hidl.data()->isUserDefinedKeyword.data(),
                                num_kw * sizeof(bool));
                    }
                    if (ret_hdr_hidl.data()->userKeywordPairFlags.data()) {
                        pListenSoundModelHeader->userKeywordPairFlags =
                            (uint16_t **)calloc(num_users, sizeof(uint16_t*));
                        if (pListenSoundModelHeader->userKeywordPairFlags) {
                            for (uint16_t u = 0; u < num_users; u++) {
                                pListenSoundModelHeader->userKeywordPairFlags[u] =
                                    (uint16_t *)calloc(num_kw, sizeof(uint16_t));
                                if (pListenSoundModelHeader->userKeywordPairFlags[u]) {
                                    for (uint16_t k = 0; k < num_kw; k++)
                                        pListenSoundModelHeader->userKeywordPairFlags[u][k] =
                                            ret_hdr_hidl.data()->userKeywordPairFlags.data()[u][k];
                                }
                            }
                        }
                    }
                    pListenSoundModelHeader->model_indicator =
                        ret_hdr_hidl.data()->model_indicator;
                }
                status = (listen_status_enum) status_;
            }
            );
    }

    ALOGD("%s:%d: status:%d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_releaseSoundModelHeader(
    listen_sound_model_header *pListenSoundModelHeader)
{
    listen_status_enum status = kFailed;
    uint16_t num_kw = 0, num_users = 0;
    uint32_t status_;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pListenSoundModelHeader) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<ListenSoundModelHeader> sm_hdr_hidl;
        num_kw = pListenSoundModelHeader->numKeywords;
        num_users = pListenSoundModelHeader->numUsers;

        sm_hdr_hidl.resize(1);
        sm_hdr_hidl.data()->numKeywords = num_kw;
        sm_hdr_hidl.data()->numUsers = num_users;
        sm_hdr_hidl.data()->numActiveUserKeywordPairs =
            pListenSoundModelHeader->numActiveUserKeywordPairs;
        sm_hdr_hidl.data()->isStripped = pListenSoundModelHeader->isStripped;
        if (pListenSoundModelHeader->langPerKw) {
            sm_hdr_hidl.data()->langPerKw.resize(num_kw);
            memcpy(sm_hdr_hidl.data()->langPerKw.data(),
                pListenSoundModelHeader->langPerKw, num_kw * sizeof(uint16_t));
        }
        if (pListenSoundModelHeader->numUsersSetPerKw) {
            sm_hdr_hidl.data()->numUsersSetPerKw.resize(num_kw);
            memcpy(sm_hdr_hidl.data()->numUsersSetPerKw.data(),
                pListenSoundModelHeader->numUsersSetPerKw, num_kw * sizeof(uint16_t));
        }
        if (pListenSoundModelHeader->isUserDefinedKeyword) {
            sm_hdr_hidl.data()->isUserDefinedKeyword.resize(num_kw);
            memcpy(sm_hdr_hidl.data()->isUserDefinedKeyword.data(),
                pListenSoundModelHeader->isUserDefinedKeyword, num_kw * sizeof(bool));
        }
        if (pListenSoundModelHeader->userKeywordPairFlags) {
            sm_hdr_hidl.data()->userKeywordPairFlags.resize(num_users);
            for (uint16_t u = 0; u < num_users; u++) {
                sm_hdr_hidl.data()->userKeywordPairFlags[u].resize(num_kw);
                for (uint16_t k = 0; k < num_kw; k++) {
                    sm_hdr_hidl.data()->userKeywordPairFlags[u][k] =
                       pListenSoundModelHeader->userKeywordPairFlags[u][k];
                }
            }
        }
        sm_hdr_hidl.data()->model_indicator = pListenSoundModelHeader->model_indicator;

        status_ = lsm_client->ipc_lsm_releaseSoundModelHeader(sm_hdr_hidl);
        status = (listen_status_enum) status_;
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getKeywordPhrases(listen_model_type *pSoundModel,
    uint16_t *numKeywords, keywordId_t *keywords)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pSoundModel || !numKeywords || !keywords) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        uint16_t inp_num_keywords = *numKeywords;
        hidl_memory model_mem;
        ret = getHidlMemory(pSoundModel->data, pSoundModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getKeywordPhrases(model_mem, pSoundModel->size, inp_num_keywords,
            [&](uint32_t status_, uint16_t numKeyWordsRet,
            hidl_vec<android::hardware::hidl_string> ret_hidl) {
                if (!status_) {
                    *numKeywords = numKeyWordsRet;
                    for (int32_t i = 0; i < numKeyWordsRet; i++) {
                        strlcpy(keywords[i],
                            ret_hidl.data()[i].c_str(),
                            MAX_STRING_LEN);
                        ALOGD("%s: keyword[%d] - %s", __func__, i, keywords[i]);
                    }
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getUserNames(listen_model_type *pSoundModel,
    uint16_t *numUsers, userId_t *users)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pSoundModel || !numUsers || !users)
        return status;
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        uint16_t inp_num_users = *numUsers;
        hidl_memory model_mem;
        ret = getHidlMemory(pSoundModel->data, pSoundModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getUserNames(model_mem, pSoundModel->size, inp_num_users,
            [&](uint32_t status_, uint16_t numUsersRet,
            hidl_vec<android::hardware::hidl_string> ret_hidl) {
                if (!status_) {
                    *numUsers = numUsersRet;
                    for (int32_t i = 0; i < numUsersRet; i++)
                        strlcpy(users[i],
                            ret_hidl.data()[i].c_str(),
                            MAX_STRING_LEN);
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_loadConfParams(uint8_t *pConfData, uint32_t confDataSize)
{
    listen_status_enum status = kFailed;
    uint32_t status_ = 0;

    ALOGD("%s", __func__);
    if (!pConfData) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<uint8_t> conf_data_hidl;

        conf_data_hidl.resize(confDataSize);
        memcpy(conf_data_hidl.data(), pConfData, confDataSize);

        status_ = lsm_client->ipc_lsm_loadConfParams(
            conf_data_hidl, confDataSize);
        status = (listen_status_enum) status_;
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getBinaryModelSize(listen_model_type *pListenModel,
    uint32_t *nBinaryModelSize)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pListenModel || !nBinaryModelSize) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pListenModel->data, pListenModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getBinaryModelSize(model_mem, pListenModel->size,
            [&](uint32_t status_, uint32_t modelSizeRet) {
                if (!status_)
                    *nBinaryModelSize = modelSizeRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_getSortedKeywordStatesUserKeywordModelSize(
    listen_model_type *pModel, uint32_t *nSortedModelSize)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pModel || !nSortedModelSize) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pModel->data, pModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_getSortedKeywordStatesUserKeywordModelSize(model_mem,
            pModel->size,
            [&](uint32_t status_, uint32_t modelSizeRet) {
                if (!status_)
                    *nSortedModelSize = modelSizeRet;
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_sortKeywordStatesOfUserKeywordModel(
    listen_model_type *pInputModel, listen_model_type *pSortedModel)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pInputModel || !pSortedModel) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_memory model_mem;
        ret = getHidlMemory(pInputModel->data, pInputModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_sortKeywordStatesOfUserKeywordModel(model_mem,
            pInputModel->size, pSortedModel->size,
            [&](uint32_t status_, hidl_memory retModelMem) {
                int32_t ret = 0;
                if (!status_) {
                    ret = mapFromHidlMemory(pSortedModel->data,
                        pSortedModel->size, retModelMem);
                    if (ret < 0)
                        status_ = kFailed;
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_verifyUserRecordingExt(listen_model_type *pKeywordModel,
    keywordId_t keywordId, listen_epd_params *pEpdParameter,
    listen_user_recording *pUserRecording, uint32_t isNoisySample,
    qualityCheckResult_t *pQualityCheckResult)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pKeywordModel || !pUserRecording || !pQualityCheckResult) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        std::string keyId(keywordId);
        hidl_vec<ListenEpdParams> epd_params_hidl;

        if (pEpdParameter) {
            epd_params_hidl.resize(1);
            memcpy(epd_params_hidl.data(), pEpdParameter, sizeof(ListenEpdParams));
        }

        hidl_memory model_mem, rec_mem;
        ret = getHidlMemory(pKeywordModel->data, pKeywordModel->size, model_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        ret = getHidlMemory(pUserRecording->data,
            pUserRecording->n_samples * sizeof(int16_t), rec_mem);
        if (ret < 0) {
            ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
            return status;
        }

        lsm_client->ipc_lsm_verifyUserRecordingExt(model_mem, pKeywordModel->size,
            keyId, epd_params_hidl, rec_mem,
            pUserRecording->n_samples * sizeof(int16_t),
            isNoisySample,
            [&](uint32_t status_,
            hidl_vec<ListenQualityCheckResult> retQualityResult) {
                if (!status_)
                    memcpy(pQualityCheckResult,
                        retQualityResult.data(),
                        sizeof(ListenQualityCheckResult));
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_smlInit(sml_t **pSmlModel, uint32_t configId,
    int8_t *pConfig, uint32_t configStructSize, int8_t *pStaticMemory,
    uint32_t staticMemorySize)
{
    listen_status_enum status = kFailed;
    int32_t ret = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<int8_t> config_hidl, static_mem_hidl;
        if (pConfig) {
            config_hidl.resize(configStructSize);
            memcpy(config_hidl.data(), pConfig, configStructSize);
        }
        if (pStaticMemory) {
            static_mem_hidl.resize(staticMemorySize);
            memcpy(static_mem_hidl.data(), pStaticMemory, staticMemorySize);
        }

        lsm_client->ipc_lsm_smlInit(configId, config_hidl, configStructSize,
            static_mem_hidl, staticMemorySize,
            [&](uint32_t status_, hidl_vec<ListenSmlModel> ret_model_hidl) {
                if (!status_) {
                   if (configId == SML_CONFIG_ID_ONLINE_VAD_INIT) {
                       listen_epd_module epd_module;
                       epd_module.indicator =
                           ret_model_hidl.data()->epdModule().indicator;
                       epd_module.pEPD =
                           (uint8_t *)ret_model_hidl.data()->epdModule().pEPD;
                       if (ret_model_hidl.data()->epdModule().tempDataSize > 0) {
                           epd_module.pTempData.size =
                               ret_model_hidl.data()->epdModule().tempDataSize;
                           ret = mapFromHidlMemory(epd_module.pTempData.data,
                                ret_model_hidl.data()->epdModule().tempDataSize,
                                ret_model_hidl.data()->epdModule().pTempData);
                           if (ret < 0)
                              status = kFailed;
                       }
                       *pSmlModel = (sml_t *)&epd_module;
                   }
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_smlSet(sml_t *pSmlModel, uint32_t configId,
    int8_t *pConfig, uint32_t configSize)
{
    listen_status_enum status = kFailed;
    uint32_t status_ = 0;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pConfig || configSize == 0) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<ListenSmlModel> sml_model_hidl;
        switch (configId) {
        case SML_CONFIG_ID_ONLINE_VAD_REINIT:
        case SML_CONFIG_ID_ONLINE_VAD_SET_PARAM:
        case SML_CONFIG_ID_ONLINE_VAD_RELEASE:
            if (pSmlModel) {
                listen_epd_module* epd_module = (listen_epd_module*) pSmlModel;
                sml_model_hidl.resize(1);
                sml_model_hidl.data()->epdModule().indicator =
                    epd_module->indicator;
                if (epd_module->pTempData.data && epd_module->pTempData.size > 0) {
                    int ret = 0;
                    hidl_memory temp_data_mem;
                    ret = getHidlMemory(epd_module->pTempData.data,
                        epd_module->pTempData.size, temp_data_mem);
                    if (ret < 0) {
                        ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                        status = kFailed;
                    }
                    sml_model_hidl.data()->epdModule().pTempData =
                        temp_data_mem;
                    sml_model_hidl.data()->epdModule().tempDataSize =
                        epd_module->pTempData.size;
                }
                sml_model_hidl.data()->epdModule().pEPD =
                    (uint64_t)epd_module->pEPD;
            } else {
                ALOGE("%s: pSmlModel is NULL", __func__);
                return status;
            }
        break;
        case SML_CONFIG_ID_SET_PDK_PARAMS:
        case SML_CONFIG_ID_SET_UDK_PARAMS:
            ALOGV("%s: No need to fill SML model mem for %d", __func__, configId);
        break;
        default:
            ALOGE("%s: Invalid config Id: %d", __func__, configId);
            break;
        }

        hidl_vec<int8_t> config_hidl;
        config_hidl.resize(configSize);
        memcpy(config_hidl.data(), pConfig, configSize);

        status_ = lsm_client->ipc_lsm_smlSet(sml_model_hidl, configId,
                      config_hidl, configSize);
        status = (listen_status_enum) status_;
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_smlGet(sml_t *pSmlModel, uint32_t configId, int8_t *pConfig,
    uint32_t configSize, uint32_t *getConfigFilledSize)
{
    listen_status_enum status = kFailed;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (!pConfig || configSize == 0 || !getConfigFilledSize) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<ListenSmlModel> sml_model_hidl;
        switch (configId) {
        case SML_CONFIG_ID_ONLINE_VAD_GET_RESULT:
            if (pSmlModel) {
                listen_epd_module* epd_module = (listen_epd_module*) pSmlModel;
                sml_model_hidl.resize(1);
                sml_model_hidl.data()->epdModule().indicator =
                    epd_module->indicator;
                if (epd_module->pTempData.data && epd_module->pTempData.size > 0) {
                    int ret = 0;
                    hidl_memory temp_data_mem;
                    ret = getHidlMemory(epd_module->pTempData.data,
                        epd_module->pTempData.size, temp_data_mem);
                    if (ret < 0) {
                        ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                        status = kFailed;
                    }
                    sml_model_hidl.data()->epdModule().pTempData =
                        temp_data_mem;
                    sml_model_hidl.data()->epdModule().tempDataSize =
                        epd_module->pTempData.size;
                }
                sml_model_hidl.data()->epdModule().pEPD =
                    (uint64_t)epd_module->pEPD;
            } else {
                ALOGE("%s: pSmlModel is NULL", __func__);
                return status;
            }
        break;
        case SML_CONFIG_ID_PARSING:
            if (pSmlModel) {
                listen_model_type *input_model = (listen_model_type *)pSmlModel;
                if (input_model->data && input_model->size > 0) {
                    sml_model_hidl.resize(1);
                    sml_model_hidl.data()->modelType().size =
                        input_model->size;
                    int ret = 0;
                    hidl_memory temp_data_mem;
                    ret = getHidlMemory(input_model->data,
                        input_model->size, temp_data_mem);
                    if (ret < 0) {
                        ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                        status = kFailed;
                    }
                    sml_model_hidl.data()->modelType().data =
                        temp_data_mem;
                }
            } else {
                ALOGE("%s: pSmlModel is NULL", __func__);
                return status;
            }
        break;
        case SML_CONFIG_ID_VERSION:
            ALOGV("%s: No pSmlModel is expected", __func__);
        break;
        default:
            ALOGE("%s: Invalid config Id: %d", __func__, configId);
        break;
        }

        hidl_vec<int8_t> config_hidl;
        if (pConfig && configSize > 0) {
            config_hidl.resize(configSize);
            memcpy(config_hidl.data(), pConfig, configSize);
        }

        lsm_client->ipc_lsm_smlGet(sml_model_hidl, configId, config_hidl,
            configSize,
            [&](uint32_t status_, hidl_vec<int8_t> ret_config_hidl,
            uint32_t retConfigSize) {
                if (!status_) {
                    *getConfigFilledSize = retConfigSize;
                    if (ret_config_hidl.data())
                        memcpy(pConfig, ret_config_hidl.data(), retConfigSize);
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}

listen_status_enum lsm_smlProcess(sml_t *pSmlModel, int8_t *pOutputData,
    int8_t *pInputData, uint32_t outputDataSize __unused, uint32_t inputDataSize,
    uint32_t outputDataId __unused, uint32_t inputDataId)
{
    listen_status_enum status = kFailed;

    ALOGD("%s:%d Enter", __func__, __LINE__);
    if (inputDataSize == 0 || !pInputData) {
        ALOGE("%s:%d Invalid input arguments", __func__, __LINE__);
        return kBadParam;
    }
    if (!lsm_server_died) {
        android::sp<IListenSoundModel> lsm_client = getLsmServer();
        if (lsm_client == NULL)
            return status;

        hidl_vec<ListenSmlModel> sml_model_hidl;
        switch (inputDataId) {
        case SML_CONFIG_ID_ONLINE_VAD_PROCESS:
            if (pSmlModel) {
                listen_epd_module* epd_module = (listen_epd_module*) pSmlModel;
                sml_model_hidl.resize(1);
                sml_model_hidl.data()->epdModule().indicator =
                    epd_module->indicator;
                if (epd_module->pTempData.data && epd_module->pTempData.size > 0) {
                    int ret = 0;
                    hidl_memory temp_data_mem;
                    ret = getHidlMemory(epd_module->pTempData.data,
                        epd_module->pTempData.size, temp_data_mem);
                    if (ret < 0) {
                        ALOGE("%s: Cannot obtain hidl memory: %d", __func__, ret);
                        status = kFailed;
                    }
                    sml_model_hidl.data()->epdModule().pTempData =
                        temp_data_mem;
                    sml_model_hidl.data()->epdModule().tempDataSize =
                        epd_module->pTempData.size;
                }
                sml_model_hidl.data()->epdModule().pEPD =
                    (uint64_t)epd_module->pEPD;
            } else {
                ALOGE("%s: pSmlModel is NULL", __func__);
                return status;
            }
        break;
        default:
            ALOGE("%s: Invalid config Id: %d", __func__, inputDataId);
        break;
        }

        hidl_vec<int8_t> input_data_hidl;
        input_data_hidl.resize(inputDataSize);
        memcpy(input_data_hidl.data(), pInputData, inputDataSize);

        lsm_client->ipc_lsm_smlProcess(sml_model_hidl, input_data_hidl,
            inputDataSize, inputDataId,
            [&](uint32_t status_, hidl_vec<int8_t> ret_output_hidl,
            uint32_t outDataSize, uint32_t outDataId __unused) {
                if (!status_) {
                    /* Update the output data size and ID if they are used in future */
                    if (ret_output_hidl.data())
                        memcpy(pOutputData, ret_output_hidl.data(), outDataSize);
                }
                status = (listen_status_enum) status_;
            }
            );
    }
    ALOGD("%s:%d: Exit, status: %d", __func__, __LINE__, status);
    return status;
}
