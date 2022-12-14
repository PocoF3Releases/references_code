/*
 * Copyright (c) 2021, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#pragma once

#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <ListenSoundModelLib.h>

using ListenStatusEnum =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenStatusEnum;
using ListenModelEnum =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenModelEnum;
using ListenDetectionStatusEnum =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenDetectionStatusEnum;
using ListenDetectionTypeEnum =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenDetectionTypeEnum;
using ListenSoundModelInfo =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenSoundModelInfo;
using ListenEventPayload =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenEventPayload;
using ListenSoundModelHeader =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenSoundModelHeader;
using ListenConfidenceLevels =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenConfidenceLevels;
using ListenDetectionEventV1 =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenDetectionEventV1;
using ListenDetectionEventV2 =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenDetectionEventV2;
using ListenDetectionEventType =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenDetectionEventType;
using ListenEpdParams =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenEpdParams;
using ListenQualityCheckResult =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenQualityCheckResult;
using ListenEpdModule =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenEpdModule;
using ListenModelType =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenModelType;
using ListenSmlModel =
    ::vendor::qti::hardware::ListenSoundModel::V1_0::ListenSmlModel;
using android::hardware::hidl_handle;
using android::hardware::hidl_memory;

class server_death_notifier : public android::hardware::hidl_death_recipient
{
    public:
        server_death_notifier() {}
        void serviceDied(uint64_t cookie,
            const android::wp<::android::hidl::base::V1_0::IBase>& who) override;
};

class client_death_notifier : public android::hardware::hidl_death_recipient
{
    public:
        client_death_notifier() {}
        void serviceDied(uint64_t cookie,
             const android::wp<::android::hidl::base::V1_0::IBase>& who) override;
};
