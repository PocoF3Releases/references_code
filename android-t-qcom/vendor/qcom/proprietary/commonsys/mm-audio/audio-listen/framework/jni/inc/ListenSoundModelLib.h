/*============================================================================
* Copyright (c) 2017-2022 Qualcomm Technologies, Inc.                        *
* All Rights Reserved.                                                       *
* Confidential and Proprietary - Qualcomm Technologies, Inc.                 *
* ===========================================================================*/


/*======================================================================
DESCRIPTION : ListenSoundModelLibrary Version 7
====================================================================*/

#ifndef __LISTEN_SOUND_MODEL_LIB_V7_H__
#define __LISTEN_SOUND_MODEL_LIB_V7_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#if defined(ANDROID)
#include <android/log.h>
#define LOGTAG ("JNILOG")
#define LOGV(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOGTAG, __VA_ARGS__)
#define LOGD(...)   __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, __VA_ARGS__)
#define LOGI(...)   __android_log_print(ANDROID_LOG_INFO, LOGTAG, __VA_ARGS__)
#define LOGW(...)   __android_log_print(ANDROID_LOG_WARN, LOGTAG, __VA_ARGS__)
#define LOGE(...)   __android_log_print(ANDROID_LOG_ERROR, LOGTAG, __VA_ARGS__)
#else
#include <stdio.h>
#define LOGE printf
#define LOGD printf
#endif

#if defined(_SML_NO_DLL)
#define DllFunc
#define CDECLARE
#elif defined(_MSC_VER) && defined(_EXPORT)
#define DllFunc   __declspec( dllexport )
#define CDECLARE    __cdecl
#elif defined(_MSC_VER)
#define DllFunc   __declspec( dllimport )
#define CDECLARE    __cdecl
#else   // For other compiler like gcc
#define DllFunc
#define CDECLARE
#endif

#define LSMLIB_VERSION 7 // %%% some unique number that changes when API changes
                         // %%% could be == SoundModel version supported for SVA 2.0

#define MAX_STRING_LEN (100 * 2)  // maximum byte size of string representing Unicode character string
// %%% NOTE: "MAX_STRING_LEN" replaces old constant "MAX_KEYWORD"

/* Version format:
external_major.external_minor.major.minor.revision.custom_alphabet.custom_number.reserved)
(8.8.8.8.8.8.8.8 bits) */
#define SML_VERSION 0x0700040100000000  // lib version 7.0_4.1.0
#define SML_COMBINED_MODEL_MAX_MODEL_NUM 5



// SVA 2.0
// Keyword & User Identifier as zero terminated strings
typedef char * keywordId_t;
typedef char * userId_t;
typedef uint8_t * sml_t;


typedef struct {
    //state machine parameters
    float minSnrOnset;                  // The minimum snr of frame that speech segment starts
    float minSnrLeave;                  // The minimum snr of frame that speech segment ends
    float snrFloor;                     // The minimum snr value assumed in the end point detector
    float snrThresholds;                // The minimum snr value of speech to verify
    float forgettingFactorNoise;        // The forgetting factor used for noise estimation
    int numFrameTransientFrame;         // the number of frames in the beginning that are used for noise estimate(valid only for online mode)
    float minEnergyFrameRatio;          // the number of frame are used for noise estimation = minenergyFrameRatio * #frames of input(valid only for batch mode)
    float minNoiseEnergy;

    //post processing parameters
    //Note:
    int numMinFramesInPhrase;           // the minimum nubmer of samples for a speech phrase (targetted speech)
    int numMinFramesInSpeech;           // [Unused] the minimum number of samples for a speech intereval
    int numMaxFrameInSpeechGap;         // the maximum allowable number of samples for a speech gap
    int numFramesInHead;                // the speech head
    int numFramesInTail;                // the speech tail

    //feature
    int preEmphasize;

    //corner case protection
    int numMaxFrames;

    // for 1st stage EPD
    int keyword_threshold;

    int reserved[4];
} listen_epd_params;

typedef struct {
    int16_t *data;              /* Audio samples ( in Raw PCM format: 16kHz, 16bit, mono ) */
    uint32_t n_samples;         /* number of audio samples */
} listen_user_recording;

typedef struct {
    uint8_t *data;              /* block of memory containing Model data */
    uint32_t size;              /* size of memory allocated for Model data */
} listen_language_model_type;

//Added for UDK RNN
typedef struct {
    uint8_t *data;              /* block of memory containing Model data */
    uint32_t size;              /* size of memory allocated for Model data */
    uint32_t type;
} listen_single_language_model_type;

typedef struct {
    uint8_t *data;              /* block of memory containing Model data */
    uint32_t size;              /* size of memory allocated for Model data */
} listen_model_type;

typedef struct {
    uint32_t indicator;
    listen_model_type pTempData;
    uint8_t *pEPD;
} listen_epd_module;

// %%% the numbering - names are up to you, as long as type + version is unique between SVA 1.0 and 2.0 SMs
typedef enum {
    kKeywordModel = 1,              // Keyword model
    kUserKeywordModel = 2,          // Userkeyword model
    kTargetSoundModel = 3,
    kMultiUserKeywordModel = 4,     // Multiple Keyword models
    kKeywordModelWithVop = 5,       // 1st Stage KW/User model + 2nd Stage User model
    kSecondStageKeywordModel = 6,   // 1st/2nd Stage Combined Keyword Model
    kSecondStageKeywordModelWithVop = 7,  // 1st/2nd Stage Combined UserKeyword
} listen_model_enum;

typedef enum {
    kSuccess = 0,
    kFailure = 1
} listen_detection_status_enum;


typedef enum _SML_CONFIG_STRUCTURE_ID {
    SML_CONFIG_ID_VERSION,
    SML_CONFIG_ID_PARSING,
    SML_CONFIG_ID_ONLINE_VAD_INIT,
    SML_CONFIG_ID_ONLINE_VAD_REINIT,
    SML_CONFIG_ID_ONLINE_VAD_SET_PARAM,
    SML_CONFIG_ID_ONLINE_VAD_PROCESS,
    SML_CONFIG_ID_ONLINE_VAD_GET_RESULT,
    SML_CONFIG_ID_ONLINE_VAD_RELEASE,
    SML_CONFIG_ID_SET_PDK_PARAMS,
    SML_CONFIG_ID_SET_UDK_PARAMS,
    SML_CONFIG_ID_RESERVED,
} SML_CONFIG_STRUCTURE_ID;


typedef enum _SML_MODEL_TYPE_ID {
    SML_MODEL_ID_NONE                 = 0x0000,
    SML_MODEL_ID_SVA_F_STAGE          = 0x0001,
    SML_MODEL_ID_SVA_S_STAGE_PDK      = 0x0002,
    SML_MODEL_ID_SVA_S_STAGE_USER     = 0x0004,
    SML_MODEL_ID_SVA_S_STAGE_RNN      = 0x0008,
    SML_MODEL_ID_SVA_S_STAGE_UBM      = 0x0010,
    SML_MODEL_ID_SVA_F_STAGE_INTERNAL = 0x0020,
    SML_MODEL_ID_SVA_S_STAGE_UDK      = 0x0040,
    SML_MODEL_ID_SVA_END              = 0x00F0,
    SML_MODEL_ID_CUSTOM_START         = 0x0100,
    SML_MODEL_ID_CUSTOM_END           = 0xF000,
} SML_MODEL_TYPE_ID;

typedef enum _SML_LANGUAGE_MODEL_TYPE_ID {
    SML_LANGUAGE_MODEL_ID_NONE = 0x0000,
    SML_LANGUAGE_MODEL_ID_GMM = 0x0001,
    SML_LANGUAGE_MODEL_ID_RNN_A = 0x0002,
    SML_LANGUAGE_MODEL_ID_RNN_T = 0x0004,
    SML_LANGUAGE_MODEL_ID_UDK7_A = 0x0008,
    SML_LANGUAGE_MODEL_ID_UDK7_U = 0x0010,
} SML_LANGUAGE_MODEL_TYPE_ID;


typedef struct {
    listen_model_enum type;     /* model type: Keyword, User, TargetSound */
    uint32_t          version;  /* model version */
    uint32_t          size;     /* total size of the model: header + payload size */
} listen_sound_model_info;

typedef struct sml_version {
    uint64_t version;
} sml_version_t;

typedef struct sml_individual_model_info{
    uint8_t *data;
    uint32_t size;
    uint32_t type;
    uint16_t versionMajor;
    uint16_t versionMinor;
}sml_individual_model_info_t;

typedef struct sml_vad_result{
    uint32_t is_detected;
    uint32_t start_index;
    uint32_t end_index;
    float snr;
}sml_vad_result_t;

typedef struct sml_parsed_info {
    uint32_t nModels = 0;
    sml_individual_model_info parsed_model[SML_COMBINED_MODEL_MAX_MODEL_NUM];
} sml_parsed_model_t;

typedef struct {
    listen_detection_status_enum status; // SUCCESS or FAILURE
    uint32_t size;               // size in bytes of payload data
                                 // just contains result confidence level values
    uint8_t *data;             // block of memory containing data payload
} listen_event_payload;

// SVA 2.0
typedef struct {
    uint16_t                numKeywords;  /* total number of keywords  */
    uint16_t                numUsers;    /* total number of users  */
    uint16_t                numActiveUserKeywordPairs;    /* total number of active user+keyword pairs in SM */
    bool                    isStripped; /* if corresponding keyword is stripped or not */
    uint16_t                *langPerKw; /* Language code of each keyword */
    /* number active Users per keyword - included as convenience */
    uint16_t                *numUsersSetPerKw;
    bool                    *isUserDefinedKeyword;
    /* Ordered 'truth' table of all possible pairs of users for each keyword.
    * Active entries marked with 1, inactive 0.keywordPhrase
    * 16-bit short (rather than boolean) is used to match SM model data size */
    uint16_t                **userKeywordPairFlags;
    uint16_t                model_indicator; /* for SM 3.0, indicate which models were combined */
} listen_sound_model_header;



// SVA 2.0
// %%% this should match the 'sensitivity' data structure input in VoiceWakeupParamType
typedef struct {
    uint8_t   size     ;  // number of keyword plus activePair confidence levels set d
    uint8_t   *pConfLevels;  // level of each keyword and each active user+keyword pair
} listen_confidence_levels ;

// SVA 2.0
typedef enum {
    kSingleKWDetectionEvent = 1,         /* SVA 1.0 model */
    kMultiKWDetectionEvent = 2,       /* SVA 2.0 model */
} listen_detection_type_enum;


// duplicates existing SVA1.0 typedef
// Do not include listen_detection_entry_v1 in SVA 1.0 header if both headers included
typedef struct {
    char keyword[MAX_STRING_LEN];
    uint16_t keywordConfidenceLevel;
    uint16_t userConfidenceLevel;
} listen_detection_event_v1;

// extends old listen_detection_entry
#define SML_MACRO_NONE "#NONE#"
// denotes that a particular entry in confidence level array is not active
static const uint8_t  NO_CONF_LEVEL = 0;

typedef struct {

    char     keywordPhrase[MAX_STRING_LEN]; /* string containing phrase string of keyword with highest confidence score */
    char     userName[MAX_STRING_LEN];  /* string containing name of user with highest confidence score */

    uint8_t  highestKeywordConfidenceLevel;  // set to zero if detection status is Failed
    uint8_t  highestUserConfidenceLevel;     // set to zero if detection status is Failed

    listen_confidence_levels  pairConfidenceLevels; // confidence levels of ALL pair (active or not)
} listen_detection_event_v2;

// modified for SVA 2.0 - this should override SVA 1.0 typedef
typedef struct {
    // %%% uint16_t version;
    listen_detection_type_enum   detection_data_type;
    // data structure filled is based on detection_data_type
    union {
        listen_detection_event_v1 event_v1;  // for SVA 1.0
        listen_detection_event_v2 event_v2;  // for SVA 2.0
    } event;
} listen_detection_event_type;

typedef enum {
    kSucess = 0,
    kFailed = 1,
    kBadParam,
    kKeywordNotFound,
    kUserNotFound,
    kUserKwPairNotActive,
    kSMVersionUnsupported,
    kUserDataForKwAlreadyPresent,
    kDuplicateKeyword,
    kDuplicateUserKeywordPair,
    kMaxKeywordsExceeded,
    kMaxUsersExceeded,
    kEventStructUnsupported,    // payload contains event data that can not be processed, or mismatches SM version
    kLastKeyword,
    kNoSignal,
    kLowSnr,
    kRecordingTooShort,
    kRecordingTooLong,
    kNeedRetrain,
    kUserUDKPairNotRemoved,
    kCannotCreateUserUDK,
    kOutputArrayTooSmall,
    kTooManyAbnormalUserScores,
    kWrongModel,
    kWrongModelAndIndicator,
    kDuplicateModel,
    kChoppedSample,
    kSecondStageKeywordNotFound,
    kClippedSample,
} listen_status_enum;

typedef struct {
    bool isEpdFilteredSegmentSet;
    bool isLowSnrSet;
    float epdSnr;
    int epdStart;                // with guard frames
    int epdEnd;                  // with guard frames
    int exactEpdStart;
    int exactEpdEnd;
    int16_t keywordConfidenceLevel;
    float epdPeakLevel;
    float epdRmsLevel;
    uint32_t n_epdSamplesClipping;
    float percentageEpdSamplesClipping;
    int keywordStart;
    int keywordEnd;
} qualityCheckResult_t;

typedef struct {
    float epdPeakLevel;
    float epdRmsLevel;
    uint32_t n_epdSamplesClipping;
    float percentageEpdSamplesClipping;
} segmentedRecordingStats_t;

#define NUM_MAX_SETTING 10
typedef struct ada_params {

    /// Noise type
    //for PDK and UDK.So far there are 4 noise types which are car / music / party / radio
    // for example, noises = { 1, 1, 0, 1, 0, 0, 0, 0, 0, 0 }, then car/ music/ radio noises will be used to noise mixing.
    // if enable_car_noise = 1; then noises[0] = 1;
    // if enable_music_noise = 1; then noises[1] = 1;
    // if enable_party_noise = 1; then noises[2] = 1;
    // if enable_radio_noise = 1; then noises[3] = 1;
    bool noises[NUM_MAX_SETTING] = { 0, };

    /// SNR settings
    // number of snrs settings in num_snrs
    // index is same order as noises variable . If snr[0] for car noise is {6, 12}, then num_snrs[0] = 2;
    int32_t numSnrs[NUM_MAX_SETTING] = { 0, };
    int32_t snr[NUM_MAX_SETTING][NUM_MAX_SETTING] = { {0, }, };

    /// Number of Clean repetition.
    //Proportion of clean sample after noise mixing
    uint32_t numCleanRepeation = 0;

    /// Effects.
    // pitch
    bool enablePitch = false;

    // tempo
    bool enableTempo = false;

    // level
    bool enableLevel = false; // give level variation based on input audio level;

    // reverb
    bool enableReverb = false;

    // noise mixing option for VoP2. we support only exising noise type in SML.
    // noise order is [car, music, party, radio] from 0 to 3.
    uint32_t noiseTypeForVoP = 2; //default is 2 which is party
    float noiseSnrForVoP = 6.0; // default is 6.0

    // reserved variable for future updates such like external noise
    uint32_t reserved[20] = { 0, };
}ada_params_t; // ADA:= Audio Data Augmentation Parameters;

typedef struct sml_pdk_params {
    /// Number of guard frames;
    // For each individual sound model such like GMM - User / Vop2 / RNN and DNN - UV during user model training
    // Guard frames for estimated start-end indices from EPD;
    // [estimated start index of EPD - numGuardFramesEPD: estimated end index of EPD + numGuardFramesEPD]
    uint32_t numGuardFramesForEPD = 30;

    // Do 1st Stage SVA EPD processing to get more precise start-end indices after SML EPD.
    // For 3.0/4.0 sound model, we will use GMMSVA's start-end indices.
    // For 5.0 sound model, we will use PDK5 XS.
    uint32_t enableFstageEpd = false;
    // Guard frames for estimated start-end indices from 1st Stage
    // [estimated start index of 1st Stage - numGuardFramesFStageEPD: estimated end index of 1st Stage + numGuardFramesFStageEPD]
    uint32_t numGuardFramesForFStageEPD = 30;

    /// check sample is front/end-chopped or not.
    // if estimated 'start index from 1st stage' < 10ms * choppingFrameThreshold; then API suspected input is front-chopped.
    // if estimated '# of frmaes - end index from 1st stage' < 10ms * choppingFrameThreshold; then API suspected input is end-chopped.
    uint32_t checkChopped = false;
    uint32_t choppingFrameThreshold = 8;

    /// snr threshold for noisy samples for ADA.
    // if you have a plan to add noisy samples for user model training.
    // it is better to increase EPD snr threshold to 11.0 ~13.0.
    float snrThresholdForNoisySamples = 6.0;

    // PCM normalization for only VoP3.
    // For SM3/4, it will be always false even we set true.
    uint32_t doPcmNormalization = false;

    // Epd params for verifyUserRecording API and createPDKModel API.
    listen_epd_params epdParams;

    // Audio Data Augmentation for PDK model training.
    uint32_t enableAda = false;
    /// number of clean samples, which are mixed with all enable case in ADA
    // maximum enrollment sample is 11.
    /// noisy recordings are mixed with limited case such like pitch/tempo/level/reverb.
    // nNoisyRecordings = 11 - nCleanRecordings; default = 6;
    uint32_t nCleanRecordings = 5;
    ada_params_t adaParam;

    /// check sample is too many clipped or not
    uint32_t checkClipped = false;
    uint32_t nClippingThreshold = 160; // 10ms, but not consecutive
    float clippingRatioThreshold = 0.004; // 0.4% of filtered sample.

    // enable zero paddings for PDK XS to get reliable result.
    // run PDK XS with 180 (unit 10ms) zero frames first, then processing actua input.
    uint32_t enableZeroProcessingForPdkXS = true;
    uint32_t nZerosPaddingFrames = 180;

    uint32_t reserved[19] = { 0, };

}sml_pdk_params_t;

typedef struct sml_udk_params {
    // Number of guard frames;
    // For 1st Stage UDK-UV, it is better to use tightly segmented vectors.
    // For 2nd Stage UV, it is better to have tolerance before/after keywords.
    uint32_t numGuardFramesForFstageUV = 0;
    uint32_t numGuradFramesForSstageUV = 30;
    // Minimum phoneme length, if length(phoneme network) < min_phoneme_length, then failed to model language sequence.
    uint32_t minPhonemeLength = 3;
    // Maximum phoneme length, if length(phoneme network) > max_phoneme_length, then failed to model language sequence.
    uint32_t maxPhonemeLength = 45;

    // PCM normalization for VoP3.
    // For SM3/4, it will be always false even we set true.
    uint32_t doPcmNormalization = false;

    // Epd params for checkUserRecording, getUDKModelSize and createUDKModel API.
    listen_epd_params epdParams;

    // Audio Data Augmentation for KW part of UDK model training
    uint32_t enableAdaForKW = false;
    ada_params_t adaParamForKW;

    // Audio Data Augmentation for UV part of UDK model training
    uint32_t enableAdaForUser = false;
    ada_params_t adaParamForUser;

    // for UDK 7.0
    uint32_t numPreGuardFramesForSstageUDK = 30;
    uint32_t numPostGuardFramesForSstageUDK = 70;
    char textInput[MAX_STRING_LEN] = { 0, }; // MAX_STRING_LEN = 200

    uint32_t reserved[20] = { 0, };
}sml_udk_params_t;

typedef struct sml_params {
    sml_pdk_params_t pdkParams;
    sml_udk_params_t udkParams;
    listen_epd_params onlineEpdParams;
} sml_params_t;

/*
*   Notes:
*       1. The client code that calls getKeywordPhrases(), getUserNames() must allocate DstStr as [MAX_STRING_LEN]
*       2. The client code that calls getUserKeywordModelSize(), createUserDefinedKeywordModel(),
*           createUserKeywordModel() should assign meaningful string for keywordId or userId, empty string is not recommended.
*       3. verifyUserRecording() should be called before calling createUserKeywordModel(). If pConfidenceLevel returned
*           in verifyUserRecording() is below a CONFIDENCE_THRESHOLD value, the recording should be rejected and not used
*           to a create user model. The client code should decide the CONFIDENCE_THRESHOLD value, a recommended value range for
*           CONFIDENCE_THRESHOLD is 60~70.
*
*/

    /*
    * findKeywordEndPosition
    *
    * Returns the keyword end position of user recordings from the keyword model
    * the keyword model finds the keyword end position by using keyword end finding algorithm inside SVA
    *
    * Param [in]  pKeywordModel - pointer to keyword model data which will be used to find keyword end
    * Param [in]  keywordId - null terminated string contains keyword phrase string
    * Param [in]  pUserRecording - a single recording of user speaking keyword
                                   other speech such as following command may follows
    * Param [out] pKendPosition - returns keyword end position from the start of user recording in number of samples
    *
    * Return - status
    *       kBadParam - When any input pointer (except pEpdParameter) is NULL
    *       kKeywordNotFound - When keywordId not exist in the model
    *       kSMVersionUnsupported - When pKeywordModel is not 2.0 model
    */
DllFunc listen_status_enum CDECLARE findKeywordEndPosition(
                listen_model_type           *pKeywordModel,
                keywordId_t                 keywordId,
                listen_user_recording       *pUserRecording,
                uint32_t                    *pKendPosition);

    /*
    * verifyUserRecording
    *
    * Returns the confidence level ( 0 ~ 100 ) that user recording matches keyword
    * User data is to be appended for a specific keyword in the model
    * // will be updated or removed
    * if input is SM 3.0 which combiend with GMM and other sound models,
    * then parsing to GMM model and run same procedure.
    *
    * Param [in]  pKeywordModel - pointer to user-independent keyword model data
    * Param [in]  keywordId - null terminated string contains keyword phrase string
    * Param [in]  listen_epd_params - epd parameter
                                      if null is passing, default epd parameter will be used internally
    * Param [in]  pUserRecording - a single recording of user speaking keyword
    * Param [out] pConfidenceLevel - returns confidence level returned by keyword detection
    *
    * Return - status
    *       kBadParam - When any input pointer (except pEpdParameter) is NULL
    *       kKeywordNotFound - When keywordId not exist in the model
    *       kSMVersionUnsupported - When pKeywordModel is not 2.0 (have to contain GMM) model
    *       kLowSnr - When user recording is too noisy
    *       kNoSignal - When user recording is non-speech
    */


DllFunc listen_status_enum CDECLARE verifyUserRecording(
        listen_model_type           *pKeywordModel,
        keywordId_t                 keywordId, // add for SVA 2.0
        listen_epd_params           *pEpdParameter,
        listen_user_recording       *pUserRecording,
        int16_t                     *pConfidenceLevel);

/*
* verifyUserRecordingExt
*
* Returns the confidence level ( 0 ~ 100 ) that user recording matches keyword
* User data is to be appended for a specific keyword in the model
* // will be updated or removed
* if input is SM 3.0 which combiend with GMM and other sound models,
* then parsing to GMM model and run same procedure.
*
* Param [in]  pKeywordModel - pointer to user-independent keyword model data
* Param [in]  keywordId - null terminated string contains keyword phrase string
* Param [in]  listen_epd_params - epd parameter
if null is passing, default epd parameter will be used internally
* Param [in]  pUserRecording - a single recording of user speaking keyword
* Param [in]  isNoisySample - if sample is noisy one, which is latter number of 'N' clean smaple, default is 0.
* Param [out] pConfidenceLevel - returns confidence level returned by keyword detection
*
* Return - status
*       kBadParam - When any input pointer (except pEpdParameter) is NULL
*       kKeywordNotFound - When keywordId not exist in the model
*       kSMVersionUnsupported - When pKeywordModel is not 2.0 (have to contain GMM) model
*       kLowSnr - When user recording is too noisy
*       kNoSignal - When user recording is non-speech
*/


DllFunc listen_status_enum CDECLARE verifyUserRecordingExt(
    listen_model_type           *pKeywordModel,
    keywordId_t                 keywordId, // add for SVA1.5
    listen_epd_params           *pEpdParameter,
    listen_user_recording       *pUserRecording,
    uint32_t                    isNoisySample,
    qualityCheckResult_t        *pQualityCheckResult);




    /*
    * checkUserRecording
    *
    * Returns the status of user recordings that if user recording has problem with SNR(Signal Noise Ratio) and length
    *
    * Param [in]  pLanguageModel - pointer to language model
    * Param [in]  pEpdParameter - pointer to EPD parameters
    *                            Default parameter will be used if eEpdParameter is NULL
    * Param [in]  pUserRecording - User recording that is going to be tested
    * Param [out]  pOutSnr - SNR of user recording
    * Param [in]  maxPhonemeLength (optional parameter) - maximum phoneme length allowed for each user recording
    *                                                   - It is optional parameter, whose default value is 0.
    *
    * Return - status
    *       kBadParam - When any input pointer (except pEpdParameter) is NULL
    *         kLowSnr   - When user recording is too noisy
    *         kNoSignal - When user recording is non-speech
    *       kRecordingTooShort - When user recording is too short
    */
DllFunc listen_status_enum CDECLARE checkUserRecording(
        listen_language_model_type  *pLanguageModel,
        listen_epd_params           *pEpdParameter,
        listen_user_recording       *pUserRecording,
        float                       *pOutSnr,
        uint32_t                    maxPhonemeLength = 0);

    /*
    * checkRecordingsQuality
    *
    * Returns the status of the last user recording in recording array that
    * if user recording has problem with SNR(Signal Noise Ratio) and length
    * Check the consistency of the input recordings if numUserRecording > 1
    *
    * Param [in]  pLanguageModel - pointer to language model
    * Param [in]  pEpdParameter - pointer to EPD parameters
    *                            Default parameter will be used if eEpdParameter is NULL
    * Param [in]  numUserRecording - number of input recordings
    * Param [in]  pUserRecordings - User recordings those are going to be tested
    * Param [out]  pOutSnr - SNR of user recording
    *
    * Return - status
    *       kBadParam - When any input pointer (except pEpdParameter) is NULL
    *         kLowSnr   - When user recording is too noisy
    *         kNoSignal - When user recording is non-speech
    *       kRecordingTooShort - When user recording is too short
    */

DllFunc listen_status_enum CDECLARE checkRecordingsQuality(
        listen_language_model_type  *pLanguageModel,
        listen_epd_params           *pEpdParameter,
        uint32_t                     numUserRecording,
        listen_user_recording       *pUserRecordings[],
        float                       *pOutSnr);

    /*
    * tuneUserDefinedKeywordModelThreshold
    *
    * This function tunes threshold of user defined keyword.
    *
    * This function can be used when programmer want to make testing stage after training stage of user defined keyword
    * even though threshold of user defined keyword is automatically tunned when create user defined keyword,
    * this function can be useful when tune more threshold of user defined keyword
    *
    * Param [in]  pUserDefinedKeyword - pointer to user defined keyword
    * Param [in]  keywordId - keyword spell
    * Param [in]  pUserRecording - user recording from testing stage
    * Param [out]  pOutputUserDefinedKeyword - tunned user defined keyword
    *
    * Return - listen_status_enum
    * Return - status
    *       kBadParam - When any input pointer is NULL, or pUserDefinedKeyword is not UDK
    *       kKeywordNotFound - When keywordId not exist in the model
    */
DllFunc listen_status_enum CDECLARE tuneUserDefinedKeywordModelThreshold(
        listen_model_type           *pUserDefinedKeyword,
        keywordId_t                 keywordId,
        listen_user_recording       *pUserRecording,
        listen_model_type           *pOutputUserDefinedKeyword);


    /*
    * getUserDefinedKeywordSize
    *
    * Get the size required to hold user defined keyword model that extends given keyword model
    * with give user data
    *
    * Param [in]  pUserDefinedKeyword - pointer to previous user defined keyword
                                        if pUserDefinedKeyword is NULL, this will create new user defined keyword model
                                        if pUserDefinedKeyword is not NULL, this will train incrementally ( not supported now )

    * Param [in]  keywordId - keyword spell of user defined keyword
    * Param [in]  userId - user spell of user defined keyword
    * Param [in]  pEpdParameter - epd parameter which is used for chopping user recording.
                                   if eEpdParameter is NULL, default parameter will be used
    * Param [in]  numUserRecording - number of user recording
    * Param [in]  pUserRecordings[] -  multiple recording of user speaking keyword
    * Param [in]  pLanguageModel - language model
    * Param [out]  pOutputSize - pointer to where output model size will be written
    *
    * Return - listen_status_enum
    * Return - status
    *       kBadParam - When any input pointer (except pUserDefinedKeyword, pEpdParameter) is NULL, or pLanguageModel is fake
    *       kNoSignal - When user recording is non-speech
    */
DllFunc listen_status_enum CDECLARE getUserDefinedKeywordSize(
        listen_model_type           *pUserDefinedKeyword,
        keywordId_t                 keywordId,
        userId_t                    userId,
        listen_epd_params           *pEpdParameter,
        uint32_t                    numUserRecording,
        listen_user_recording       *pUserRecordings[],
        listen_language_model_type  *pLanguageModel,
        uint32_t                    *pOutputSize);


/*Added to get the size of LanguageModel + AcousticModel + ThresholdModel
* Get the size of language model, acoustic model, and threshold model
* to compute the size of merged model
* Param [in] language_model - language model for GMM UDK creation
* Param [in] acoustic_model - acoustic model for RNN UDK creation
* Param [in] threshold_model - threshold model for RNN UDK creation
* Param [out] nMergedModelSize - size of the merged model
*/
DllFunc listen_status_enum CDECLARE getMergedLanguageModelSize(
        uint32_t                          nModels,
        listen_single_language_model_type *pModels[],
        uint32_t *pMergedModelSize);

/*Added to create MergedModel
* from LanguageModel, AcousticModel, and ThresholdModel
* Param [in] language_model - language model for GMM UDK creation
* Param [in] acoustic_model - acoustic model for RNN UDK creation
* Param [in] threshold_model - threshold model for RNN UDK creation
* Param [out] pMergedModel - Merged language acoustic threshold model
*/
DllFunc listen_status_enum CDECLARE mergeLanguageModel(
        uint32_t                          nModels,
        listen_single_language_model_type *pModels[],
        listen_language_model_type *pMergedModel);

/*Added for parsing language model, acoustic model, and threshold model from the input merged_language_acoustic_threshold model
* Param [in] pMergedModel - input model contains language model, acoustic model, and threshold model
* Param [out] language_model - language model (used for GMM UDK)
* Param [out] acoustic_model - acoustic model (used for RNN UDK)
* Param [out] threshold_model - threshold model (used for RNN UDK)
*/
DllFunc listen_status_enum CDECLARE getParsedLanguageModelSizes(
        listen_language_model_type *pMergedModel,
        uint32_t model_type,
        uint32_t *pModelSize);

/*Added to get individual language model sizes
* Param [in] pMergedModel - merged_language_acoustic_threshold_model
* Param [out]  LanguageModelsize - language model size
* Param [out]  AcousticModelsize - acoustic model size
* Param [out]  ThresholdModelsize - threshold model size
*/
DllFunc listen_status_enum CDECLARE parseMergedLanguageModel(
        listen_language_model_type *pMergedModel,
        uint32_t                          nModels,
        listen_single_language_model_type *pModels[]);


    /*
    * getUserDefinedKeywordApproxSize
    *
    * Get the size required to hold user-keyword model that extends given keyword model
    * with give user data
    *
    * Param [in]  keywordId - null terminated string containing keyword phrase string
    * Param [in]  userId - null terminated string containing user name string
    * Param [in]  pMergedModel - pointer to merged model data
    * Param [out] pOutputSize - size of approximated output model
    * Param [in]  maxPhonemeLength (optional parameter) - maximum phoneme length allowed for each user recording
    *                                                   - It is optional parameter, whose default value is 0.
    *
    * Return - status
    *       kBadParam - When any input pointer is NULL
    */

DllFunc listen_status_enum CDECLARE getUserDefinedKeywordApproxSize(
        keywordId_t                     keywordId,
        userId_t                        userId,
        listen_language_model_type      *pLanguageModel,
        uint32_t                        *pOutputSize,
        uint32_t                        maxPhonemeLength = 0);

    /*
    * createUserDefinedKeywordModel
    *
    * Description : Create User Defined Keyword Model
    *
    * Param [in]  pUserDefinedKeyword - pointer to previous user defined keyword
                                        if pUserDefinedKeyword is NULL, this will create new user defined keyword model
                                        if pUserDefinedKeyword is not NULL, this will train incrementally ( not supported now )

    * Param [in]  keywordId - keyword spell of user defined keyword
    * Param [in]  userId - user spell of user defined keyword
    * Param [in]  pEpdParameter - epd parameter which is used for chopping user recording.
                                   if eEpdParameter is NULL, default parameter will be used
    * Param [in]  numUserRecording - number of user recording
    * Param [in]  pUserRecordings[] - multiple recording of user speaking keyword
    * Param [in]  pMergedModel - can contain only GMM UDK language model or GMM UDK language model + RNN UDK models( acoustic + thtreshold models)
    * Param [out] pOutputUserDefinedKeyword - pointer to where output model will be written
    * Param [out] pMatchingScore - pointer to matching score
    *
    * Return - listen_status_enum
    * Return - status
    *       kBadParam - When any input pointer (except pUserDefinedKeyword, pEpdParameter) is NULL, or pLanguageModel is fake
    *       kNoSignal - When user recording is non-speech
    *       kCannotCreateUserUDK - When creation process fails somewhere
    *       kOutputArrayTooSmall - When output size is smaller than actual udk model size
    */
DllFunc listen_status_enum CDECLARE createUserDefinedKeywordModel(
        listen_model_type           *pUserDefinedKeyword,
        keywordId_t                 keywordId,
        userId_t                    userId,
        listen_epd_params           *pEpdParameter,
        uint32_t                     numUserRecording,
        listen_user_recording       *pUserRecordings[],
        listen_language_model_type *pLanguageModel,
        listen_model_type           *pOutputUserDefinedKeyword,
        int16_t                     *pMatchingScore);

    /*
    * getStrippedUserKeywordModelSize
    *
    * Return stripped model size
    *
    * Param[in] pModel - pointer to (user)keyword model data
    * Param[out] nStrippedModelSize - return model size of stripped model
    *
    * Return - status
    *           kBadParam - When any input pointer is NULL
    *           kSMVersionUnsupported - When pModel is not 2.0 model
    *
    */
DllFunc listen_status_enum CDECLARE getStrippedUserKeywordModelSize(
        listen_model_type           *pModel,
        uint32_t                    *nStrippedModelSize);


    /*
    * stripUserKeywordModel
    *
    * Return stripped model
    *
    * Param[in] pModel - pointer to (user)keyword model data
    * Param[out] pStrippedModel - pointer to stripped model data
    *
    * Return - status
    *           kBadParam - When any input pointer is NULL
    *           kSMVersionUnsupported - When pModel is not 2.0 model
    *
    */
DllFunc listen_status_enum CDECLARE stripUserKeywordModel(
        listen_model_type           *pModel,
        listen_model_type           *pStrippedModel);

    /*
    * getUserKeywordModelSize
    *
    * Get the size required to hold user-keyword model that extends given keyword model
    * with give user data
    *
    * Param [in]  pKeywordModel - pointer to keyword model data
    * Param [in]  keywordId - null terminated string containing keyword phrase string
    * Param [in]  userId - null terminated string containing user name string
    * Param [out] nUserKeywordModelSize - size of user keyword model
    *
    * Return - status
    *       kBadParam - When any input pointer is NULL
    *       kKeywordNotFound - When keywordId not exist in the model
    *       kSMVersionUnsupported - When pKeywordModel is not 2.0/3.0 model
    */
DllFunc listen_status_enum CDECLARE getUserKeywordModelSize(
        listen_model_type           *pKeywordModel,
        keywordId_t                 keywordId, // add for SVA 2.0
        userId_t                    userId, // add for SVA 2.0
        uint32_t                    *nUserKeywordModelSize);

    /*
    * createUserKeywordModel
    *
    * Create a user keyword model
    * Writes the user keyword model into given memory location
    *
    * Param [in]  pKeywordModel - pointer to Keyword model  or User keyword model data
                                if it is keyword model, create user-keyword model
                                if it is user keyword model, incrementally train user keyword model
    * Param [in]  keywordId - user data is to be appended for keyword in model with given identifier
    * Param [in]  userId - identifier of user data is created
    *            If identifier is already used, will replace existing user data with newly created data.
    *            The User Name is passed to this function so that if this is the first time user data is
    *            being added for a new user, the User's Name can be stored in the SM
    * Param [in]  pEpdParameter - end point detection parameter
    *                             if eEpdParameter is NULL, default parameter will be used
    * Param [in]  numUserRecording - number of user recordings
    * Param [in]  pUserRecordings - multiple recording of user speaking keyword
    * Param [out] pUserKeywordModel - pointer to where user keyword model data is to be written
    * Param [out] pUserMatchingScore - pointer to user matching score
    * Return - status
    *       kBadParam - When any input pointer (except pEpdParameter) is NULL
    *       kKeywordNotFound - When keywordId not exist in the model
    *       kSMVersionUnsupported - When pKeywordModel is not 2.0 or 3.0 model
    *        kLowSnr    - When user recording is too noisy
    *        kNoSignal - When user recording is non-speech
    *       kCannotCreateUserUDK - When pKeywordModel is UDK model
    */
DllFunc listen_status_enum CDECLARE createUserKeywordModel(
        listen_model_type           *pKeywordModel,
        keywordId_t                 keywordId, // add for SVA 2.0
        userId_t                    userId, // add for SVA 2.0
        listen_epd_params           *pEpdParameter,
        uint32_t                    numUserRecording,
        listen_user_recording       *pUserRecordings[],
        listen_model_type           *pUserKeywordModel,
        int16_t                     *pUserMatchingScore);

// Since size of new SM after removing data will be less than or equal to size of
// input SM, this function could be optional and size of pInputModel could be used
// to allocate memory for pResultModel when deleteFromModel() called.
    /*
    * getSizeAfterDeleting
    *
    * Return the size of sound model after removing data from a given SM for either
    * a keyword, a user, or a specific user+keyword pair.
    *
    * Param [in]  pInputModel - pointer to sound model
    * Param [in]  keywordId - data for this keyword in model with given identifier is removed
    *           If userId is 'null', then all keyword-only data and all user data associated
    *           with the given non-null keywordId is removed.
    *           If userId is also non-null, then only data associated with the userId+keywordId
    *           pair is removed.
    * Param [in]  userId - all data for this user in model with given identifier is removed
    *           If keywordId is 'null', then all all user data for the given non-null userId
    *           is removed.
    *           If keywordId is also non-null, then only data associated with the userId+keywordId
    *           pair is removed.
    * Param [out]  nOutputModelSize - outputs size of resulting soundmodel after removing data.
    * Return - status
    *       kBadParam - When any input pointer (except keywordId, userId) is NULL
    *       kLastKeyword - When pInputModel has only one keyword
    *       kSMVersionUnsupported - When pInputModel is not 2.0 model
    *       kKeywordNotFound - When keywordId not exist in the model
    *       kUserNotFound - When userId not exist in the model
    *       kUserKWPairNotActive - When <keywordId, userId> pair not exist in the model
    *       kUserUDKPairNotRemoved - When <keywordId, userId> pair to delete is UDK
    */
DllFunc listen_status_enum CDECLARE getSizeAfterDeleting(
        listen_model_type           *pInputModel,
        keywordId_t                 keywordId, // add for SVA 2.0
        userId_t                    userId, // add for SVA 2.0
        uint32_t                    *nOutputModelSize);

// If getSizeAfterDeleting() supported, call it get size of new sound model after
// removing desired data from given input sound model, and
// allocate ResultModel with this size
// Otherwise, use size of input SoundModel since size of ResultModel will be
// less than or equal to size of input SoundModel.
    /*
    * deleteFromModel
    *
    * Return a new sound model after removing data from a given SM for a keyword, a user,
    * or a user+keyword pair.
    *
    * Param [in]  pInputModel - pointer to sound model
    * Param [in]  keywordId - data for this keyword in model with given identifier is removed
    *           If userId is 'null', then all keyword-only data and all user data associated
    *           with the given non-null keywordId is removed.
    *           If userId is also non-null, then only data associated with the userId+keywordId
    *           pair is removed.
    * Param [in]  userId - all data for this user in model with given identifier is removed
    *           If keywordId is 'null', then all all user data for the given non-null userId
    *           is removed.
    *           If keywordId is also non-null, then only data associated with the userId+keywordId
    *           pair is removed.
    * Param [out]  pResultModel - pointer to where user keyword model data is to be written
    * Return - status
    *       kBadParam - When any input pointer (except keywordId, userId) is NULL
    *       kLastKeyword - When pInputModel has only one keyword
    *       kSMVersionUnsupported - When pInputModel is not 2.0 or 3.0 model
    *       kKeywordNotFound - When keywordId not exist in the model
    *       kUserNotFound - When userId not exist in the model
    *       kUserKWPairNotActive - When <keywordId, userId> pair not exist in the model
    *       kUserUDKPairNotRemoved - When <keywordId, userId> pair to delete is UDK
    */
DllFunc listen_status_enum CDECLARE deleteFromModel(
        listen_model_type           *pInputModel,
        keywordId_t                 keywordId, // add for SVA 2.0
        userId_t                    userId, // add for SVA 2.0
        listen_model_type           *pResultModel);


    /*
    * getMergedModelSize
    *
    * Return the size of sound model after merging required models
    *
    * Param [in]  numModels - number of model files to be merged
    * Param [in]  pModels - array of pointers to Keyword or User keyword model data
    * Param [out]  nOutputModelSize - outputs size of resulting soundmodel after merging models
    *
    * Return - status
    *       kBadParam - When any input pointer is NULL
    *       kSMVersionUnsupported - When pInputModel is not 2.0 model
    *       kDuplicateKeyword - When same keywordId exists in more than 1 model
    *       kDuplicateUserKeywordPair
    *       kMaxKeywordsExceeded
    *       kMaxUsersExceeded,
    */
DllFunc listen_status_enum CDECLARE getMergedModelSize(
        uint16_t                    numModels,
        listen_model_type           *pModels[],
        uint32_t                    *nOutputModelSize);



    /*
    * mergeModels
    *
    * merges two or more Sound Models
    *
    * Writes the new merged model into given memory location
    *
    * Param [in]  numModels - number of model files to be merged
    * Param [in]  pModels - array of pointers to Keyword or User keyword model data
    * Param [out]  pMergedModel - pointer to where merged model data is to be written
    * Return - status
    *       kBadParam - When any input pointer is NULL
    *       kSMVersionUnsupported - When pInputModel is not 2.0 model
    *       kDuplicateKeyword - When same keywordId exists in more than 1 model
    *       kDuplicateUserKeywordPair - N/A to current version
    *       kMaxKeywordsExceeded - N/A to current version
    *       kMaxUsersExceeded - N/A to current version
    */
DllFunc listen_status_enum CDECLARE mergeModels(
        uint16_t                    numModels,
        listen_model_type       *pModels[],
        listen_model_type       *pMergedModel);


    /*
    * parseDetectionEventData
    *
    * parse event payload into detection event.
    *
    * Version of input SM will detemine DetectionType created/returned
    *
    * Param [in]  pUserKeywordModel - pointer to keyword or user keyword model data
    * Param [in]  pEventPayload - pointer to received event payload data
    * Param [out] pDetectEvent - pointer to where detection event data is to be written
    * Return - status
    *       kBadParam - When any input pointer is NULL
    *       kSMVersionUnsupported - When pUserKeywordModel is not 2.0 model
    *       kEventStructUnsupported - When pEventPayload->size != numKeywords + numActiveUsers
    */

DllFunc listen_status_enum CDECLARE parseDetectionEventData(
        listen_model_type           *pUserKeywordModel,
        listen_event_payload        *pEventPayload,
        listen_detection_event_type *pDetectionEvent);


// Declared in both SVA 1.0 and SVA 2.0 versions and SML 3.0 of ListenSoundModelMLib
//
    /*
    * querySoundModel
    *
    * Returns the information about a sound model
    * Sound model could be of any type: Keyword, UserKeyword, TargetSound,...
    * Sound model could be any versions
    *
    * Param [in] pSoundModel - pointer to model data
    * Param [out] pListenSoundModelInfo - returns information about the give sound model
    *
    * Return - status
    *       kBadParam - When any input pointer is NULL
    *       kFailed - When input model failed to be decoded
    *        kSMVersionUnsupported - When pSoundModel is fake model (invalid model other than 1.0 model, 2.0 model and 3.0 model)
    */
DllFunc listen_status_enum CDECLARE querySoundModel(
        listen_model_type           *pSoundModel,
        listen_sound_model_info     *pListenSoundModelInfo);


    /*
    * getSoundModelHeader
    *
    * Returns additional information about the sound model
    * Sound model could be of any type: Keyword, UserKeyword, TargetSound,...
    * Keyword
    *
    * Param [in] pSoundModel - pointer to model data
    * Param [out] pListenSoundModelHeader - returns header field from sound model
    *
    * Return - status
    *       kBadParam - When any input pointer is NULL
    *       kSMVersionUnsupported - When pSoundModel is not 2.0 or 3.0 model
    */

DllFunc listen_status_enum CDECLARE getSoundModelHeader(
        listen_model_type           *pSoundModel,
        listen_sound_model_header   *pListenSoundModelHeader);


    /*
    * release sound model header
    *
    * deallocate sound model header
    * Return - status
    *       kBadParam - When any input pointer is NULL
    */
DllFunc listen_status_enum CDECLARE releaseSoundModelHeader(
        listen_sound_model_header   *pListenSoundModelHeader);


    /*
    * getKeywordPhrases
    *
    * Get keyword phrase string for all Keywords defined in given SM 2.0 / 3.0
    *
    * App calling this function must allocate memory for all phrases
    * by getting the number of keywords from querySoundModel() and allocating
    * totalNumKeywords*MAX_STRING_LEN
    * This function copies phrases into this allocated keywords array
    *
    * Param [in]  pSoundModel - pointer to model data
    * Param [in/out] numKeywords - [in]  number of string entries allocated in keywords array
    *                             [out] number of keyword phrase strings copied keyword array
    * Param [out] keywords - array of keyword phrase null-terminated strings
    *
    * Return - status
    *       kBadParam - When any input pointer is NULL, or numKeywords < real keywords number
    *       kSMVersionUnsupported - When pSoundModel is not 2.0 or 3.0 model
    */
DllFunc listen_status_enum CDECLARE getKeywordPhrases(
        listen_model_type           *pSoundModel,
        uint16_t                    *numKeywords,
        keywordId_t                 *keywords);


    /*
    * getUserNames
    *
    * Get user names for user data associated with a given SM 2.0 / 3.0
    *
    * App calling this function must allocate memory for all names
    * by getting the number of users  from querySoundModel() and allocating
    * totalNumUsers*MAX_STRING_LEN
    * This function copies names into this allocated keywords array
    *
    * Param [in]  pSoundModel - pointer to model data
    * Param [in/out] numUsers - [in]  number of string entries allocated in users array
    *                          [out] number of user name strings copied users array
    * Param [out] users - array of user name null-terminated strings
    *
    * Return - status
    *       kBadParam - When any input pointer is NULL, or numUsers < real users number, or pSoundModel is keyword-only model
    *       kSMVersionUnsupported - When pSoundModel is not 2.0 or 3.0 model
    */
DllFunc listen_status_enum CDECLARE getUserNames(
        listen_model_type           *pSoundModel,
        uint16_t                        *numUsers,
        userId_t                        *users);

    /*
    * loadConfParams
    *
    * Load configurable parameters to the sound model library
    *
    * Param [in] pConfData - pointer to param data
    * Param [in] confDataSize - size of memory allocated for param data
    *
    * Return - status
    *       kBadParam - When any input pointer is NULL
    */
DllFunc listen_status_enum CDECLARE loadConfParams(
        uint8_t                     *pConfData,
        uint32_t                    confDataSize);
    /*
    * getBinaryModelSize
    *
    * Return binary model size
    *
    * Param[in] pListenModel - pointer to (user)keyword model data
    * Param[out] nBinaryModelSize - return model size of binary model
    *
    * Return - status
    *           kBadParam - When any input pointer is NULL
    *           kSMVersionUnsupported - When pModel is not 2.0 or 3.0 model
    *
    */

DllFunc listen_status_enum CDECLARE getBinaryModelSize(
        listen_model_type           *pListenModel,
        uint32_t                    *nBinaryModelSize);

    /*
    * getSortedKeywordStatesUserKeywordModelSize
    *
    * Return sorted model size
    *
    * Param[in] pModel - pointer to (user)keyword model data
    * Param[out] nSortedModelSize - return model size of sorted keyword states model
    *
    * Return - status
    *           kBadParam - When any input pointer is NULL
    *
    */

DllFunc listen_status_enum CDECLARE getSortedKeywordStatesUserKeywordModelSize(
    listen_model_type           *pModel,
    uint32_t                    *nSortedModelSize);


    /*
    * sortKeywordStatesOfUserKeywordModel
    *
    * Return sorted model
    *
    * Param[in] pInputModel - pointer to (user)keyword model data
    * Param[out] pSortedModel - pointer to sorted keyword states model data
    *
    * Return - status
    *           kBadParam - When any input pointer is NULL
    *           kSMVersionUnsupported - when pModel is not 2.0 model
    *
    */

DllFunc listen_status_enum CDECLARE sortKeywordStatesOfUserKeywordModel(
        listen_model_type           *pInputModel,
        listen_model_type           *pSortedModel);




/*
* smlInit
*
* Initialize the input binary model
*
* Param[in/ouput] pSmlModel   - pointer to initialized model.
* Param[in] configId          - Id for which model type should be initialized
                                SML_CONFIG_ID_ONLINE_VAD_INIT is only used.
* Param[in] pConfig           - Input data strcuture which is corresponding to config ID
* Param[in] configStructSize  - Size of input data structure
* Param[in] pStaticMemory     - pointer for scratch memory
* Param[in] staticMemorySize - Size of scratch memory size
*
* Return - status
*           kFailed   - When memory allocation failure
*           kBadParam - When any input pointer is NULL or
*                       input argument size is different to correspondant structure.
*           kSMVersionUnsupported - when input model is not combined.
*
*/
DllFunc listen_status_enum CDECLARE smlInit(
    sml_t            **pSmlModel,
    uint32_t         configId,
    int8_t           *pConfig,
    uint32_t         configStructSize,
    int8_t           *pStaticMemory,
    uint32_t         staticMemorySize);


/*
* smlSet
*
* Set configuration to initialized model
*
* Param[in] pSmlModel         - pointer to initialized model.
* Param[in] configId          - SML_CONFIG_STRUCTURE_ID to be used
* Param[in] pConfig           - Input data strcuture which is corresponding to config ID
* Param[in] configSize        - Size of input data structure
*
* Return - status
*           kFailed   - When memory allocation failure
*           kBadParam - When any input pointer is NULL or
*                       input argument size is different to correspondant structure.
*           kSMVersionUnsupported - when input model is not combined.
*
*/
DllFunc listen_status_enum CDECLARE smlSet(
    sml_t            *pSmlModel,
    uint32_t         configId,
    int8_t           *pConfig,
    uint32_t         configSize);


/*
* smlSet
*
* Get configuration/result from initialized model
*
* Param[in]      pSmlModel             - pointer to initialized model.
* Param[in]      configId              - SML_CONFIG_STRUCTURE_ID to be used
* Param[in/out]  pConfig               - Input/Output data strcuture, which is corresponding to config Id,
                                         to be filled
* Param[in]      configSize            - Size of input data structure
* Param[out]     getConfigFilledSize   - Size of filled structure size
*
* Return - status
*           kFailed   - When memory allocation failure
*           kBadParam - When any input pointer is NULL or
*                       input argument size is different to correspondant structure.
*           kSMVersionUnsupported - when input model is not combined.
*
*/
DllFunc listen_status_enum CDECLARE smlGet(
    sml_t            *pSmlModel,
    uint32_t         configId,
    int8_t           *pConfig,
    uint32_t         configSize,
    uint32_t         *getConfigFilledSize);




/*
* smlProcess
*
* Process input data to initialized model.
*
* Param[in]         pSmlModel          - pointer to initialized model.
* Param[out]        pOutputData        - Unused, Set to be Null/0
* Param[in]         pInputData         - pointer to input data (i.e. raw audio data)
* Param[out]        outputDataSize     - Unused, Set to be Null/0
* Param[in]         inputDataSize      - input data size
* Param[out]        outputDataId       - Unused, Set to be Null/0
* Param[in]         inputDataId        - SML_CONFIG_STRUCTURE_ID to be used
*
* Return - status
*           kFailed   - When memory allocation failure
*           kBadParam - When any input pointer is NULL or
*                       input argument size is different to correspondant structure.
*           kSMVersionUnsupported - when input model is not combined.
*
*/
DllFunc listen_status_enum CDECLARE smlProcess(
    sml_t            *pSmlModel,
    int8_t           *pOutputData,
    int8_t           *pInputData,
    uint32_t         outputDataSize,
    uint32_t         inputDataSize,
    uint32_t         outputDataId,
    uint32_t         inputDataId);



#ifdef __cplusplus
};
#endif

#endif /* __LISTEN_SOUND_MODEL_LIB_V5_H__ */

