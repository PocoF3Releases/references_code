/*
 * Copyright (c) 2021, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#pragma once

bool isLsmHidlClientSupported(void);

listen_status_enum lsm_findKeywordEndPosition(listen_model_type *pKeywordModel,
          keywordId_t keywordId, listen_user_recording *pUserRecording,
          uint32_t *pKendPosition);

listen_status_enum lsm_verifyUserRecording(listen_model_type *pKeywordModel,
          keywordId_t keywordId, listen_epd_params *pEpdParameter,
          listen_user_recording *pUserRecording, int16_t *pConfidenceLevel);

listen_status_enum lsm_checkUserRecording(
          listen_language_model_type *pLanguageModel,
          listen_epd_params *pEpdParameter,
          listen_user_recording *pUserRecording, float *pOutSnr,
          uint32_t maxPhonemeLength);

listen_status_enum lsm_checkRecordingsQuality(
          listen_language_model_type *pLanguageModel,
          listen_epd_params *pEpdParameter,
          uint32_t numUserRecording,
          listen_user_recording *pUserRecordings[], float *pOutSnr);

listen_status_enum lsm_tuneUserDefinedKeywordModelThreshold(
          listen_model_type *pUserDefinedKeyword,
          keywordId_t keywordId, listen_user_recording *pUserRecording,
          listen_model_type *pOutputUserDefinedKeyword);

listen_status_enum lsm_getUserDefinedKeywordSize(
          listen_model_type *pUserDefinedKeyword,
          keywordId_t keywordId, userId_t userId,
          listen_epd_params *pEpdParameter, uint32_t numUserRecording,
          listen_user_recording *pUserRecordings[],
          listen_language_model_type *pLanguageModel, uint32_t *pOutputSize);

listen_status_enum lsm_getUserDefinedKeywordApproxSize(
          keywordId_t keywordId, userId_t userId,
          listen_language_model_type *pLanguageModel, uint32_t *pOutputSize,
          uint32_t maxPhonemeLength);

listen_status_enum lsm_createUserDefinedKeywordModel(
          listen_model_type *pUserDefinedKeyword,
          keywordId_t keywordId, userId_t userId,
          listen_epd_params *pEpdParameter, uint32_t numUserRecording,
          listen_user_recording *pUserRecordings[],
          listen_language_model_type *pLanguageModel,
          listen_model_type *pOutputUserDefinedKeyword,
          int16_t *pMatchingScore);

listen_status_enum lsm_getStrippedUserKeywordModelSize(
          listen_model_type *pModel, uint32_t *nStrippedModelSize);

listen_status_enum lsm_stripUserKeywordModel(listen_model_type *pModel,
          listen_model_type *pStrippedModel);

listen_status_enum lsm_getUserKeywordModelSize(listen_model_type *pKeywordModel,
          keywordId_t keywordId, userId_t userId,
          uint32_t *nUserKeywordModelSize);

listen_status_enum lsm_createUserKeywordModel(
          listen_model_type *pKeywordModel, keywordId_t keywordId,
          userId_t userId, listen_epd_params *pEpdParameter,
          uint32_t numUserRecording, listen_user_recording *pUserRecordings[],
          listen_model_type *pUserKeywordModel,
          int16_t *pUserMatchingScore);

listen_status_enum lsm_getSizeAfterDeleting(listen_model_type *pInputModel,
          keywordId_t keywordId,
          userId_t userId,
          uint32_t *nOutputModelSize);

listen_status_enum lsm_deleteFromModel(listen_model_type *pInputModel,
          keywordId_t keywordId,
          userId_t userId,
          listen_model_type *pResultModel);

listen_status_enum lsm_getMergedModelSize(uint16_t numModels,
          listen_model_type *pModels[], uint32_t *nOutputModelSize);

listen_status_enum lsm_mergeModels(uint16_t numModels,
          listen_model_type *pModels[],
          listen_model_type *pMergedModel);

listen_status_enum lsm_parseFromBigSoundModel(listen_model_type *pSM3p0Model,
          listen_model_type *p1stStageModel, listen_model_type *p2ndStageKWModel,
          listen_model_type *p2stStageVoPModel, uint16_t *indicator);

listen_status_enum lsm_parseDetectionEventData(
          listen_model_type *pUserKeywordModel,
          listen_event_payload *pEventPayload,
          listen_detection_event_type *pDetectionEvent);

listen_status_enum lsm_querySoundModel(listen_model_type *pSoundModel,
          listen_sound_model_info *pListenSoundModelInfo);

listen_status_enum lsm_getSoundModelHeader(listen_model_type *pSoundModel,
          listen_sound_model_header *pListenSoundModelHeader);

listen_status_enum lsm_releaseSoundModelHeader(
       listen_sound_model_header *pListenSoundModelHeader);

listen_status_enum lsm_getKeywordPhrases(listen_model_type *pSoundModel,
          uint16_t *numKeywords, keywordId_t *keywords);

listen_status_enum lsm_getUserNames(listen_model_type *pSoundModel,
          uint16_t *numUsers, userId_t *users);

listen_status_enum lsm_loadConfParams(uint8_t *pConfData, uint32_t confDataSize);

listen_status_enum lsm_getBinaryModelSize(listen_model_type *pListenModel,
          uint32_t *nBinaryModelSize);

listen_status_enum lsm_getSortedKeywordStatesUserKeywordModelSize(
      listen_model_type *pModel, uint32_t *nSortedModelSize);

listen_status_enum lsm_sortKeywordStatesOfUserKeywordModel(
          listen_model_type *pInputModel, listen_model_type *pSortedModel);

listen_status_enum lsm_verifyUserRecordingExt(listen_model_type *pKeywordModel,
          keywordId_t keywordId, listen_epd_params *pEpdParameter,
          listen_user_recording *pUserRecording, uint32_t isNoisySample,
          qualityCheckResult_t *pQualityCheckResult);

listen_status_enum lsm_smlInit(sml_t **pSmlModel, uint32_t configId,
          int8_t *pConfig, uint32_t configStructSize, int8_t *pStaticMemory,
          uint32_t staticMemorySize);

listen_status_enum lsm_smlSet(sml_t *pSmlModel, uint32_t configId,
          int8_t *pConfig, uint32_t configSize);

listen_status_enum lsm_smlGet(sml_t *pSmlModel, uint32_t configId,
          int8_t *pConfig, uint32_t configSize, uint32_t *getConfigFilledSize);

listen_status_enum lsm_smlProcess(sml_t *pSmlModel, int8_t *pOutputData,
          int8_t *pInputData, uint32_t outputDataSize, uint32_t inputDataSize,
          uint32_t outputDataId, uint32_t inputDataId);
