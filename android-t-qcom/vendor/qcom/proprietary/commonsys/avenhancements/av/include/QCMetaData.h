/*
 * Copyright (c) 2019, 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Copyright (c) 2012-2015, 2018, The Linux Foundation. All rights reserved.
 *
 */

#ifndef QC_META_DATA_H_

#define QC_META_DATA_H_

namespace android {

enum {
    kKeyAacCodecSpecificData = 'nacc' , // for native aac files

    kKeyRawCodecSpecificData = 'rcsd',  // raw data - added to support mmParser
    kKeyDivXVersion          = 'DivX',  // int32_t
    kKeyDivXDrm              = 'QDrm',  // void *
    kKeyWMAEncodeOpt         = 'eopt',  // int32_t
    kKeyWMAEncodeOptC2       = 'eopT', // int32_t
    kKeyWMABlockAlign        = 'blka',  // int32_t
    kKeyWMABlockAlignC2      = 'blkA', // int32_t
    kKeyWMAVersion           = 'wmav',  // int32_t
    kKeyWMAVersionC2         = 'wmaV',  // int32_t
    kKeyWMAAdvEncOpt1        = 'ade1',  // int16_t
    kKeyWMAAdvEncOpt1C2      = 'adE1',  // int16_t
    kKeyWMAAdvEncOpt2        = 'ade2',  // int32_t
    kKeyWMAAdvEncOpt2C2      = 'adE2',  // int32_t
    kKeyWMAFormatTag         = 'fmtt',  // int64_t
    kKeyWMAFormatTagC2       = 'fmtT',  // int64_t
    kKeyWMABitspersample     = 'bsps',  // int64_t
    kKeyWMABitspersampleC2   = 'bspS',  // int64_t
    kKeyWMAVirPktSize        = 'vpks',  // int64_t
    kKeyWMAChannelMask       = 'chmk',  // int32_t
    kKeyWMAChannelMaskC2     = 'chmK',  // int32_t
    kKeyVorbisData           = 'vdat',  // raw data
    kKeyMHAConfig            = 'mhaC',  // raw data
    kKeyMHASceneInfo         = 'mhaS',  // raw data

    kKeyFileFormat           = 'ffmt',  // cstring

    kkeyAacFormatAdif        = 'adif',  // bool (int32_t)
    kKeyInterlace            = 'intL',  // bool (int32_t)
    kkeyAacFormatLtp         = 'ltp ',


    //DTS subtype
    kKeyDTSSubtype           = 'dtss',  //int32_t

    //Extractor sets this
    kKeyUseArbitraryMode     = 'ArbM',  //bool (int32_t)
    kKeySmoothStreaming      = 'ESmS',  //bool (int32_t)
    kKeyHFR                  = 'hfr ',  // int32_t
    kKeyHSR                  = 'hsr ',  // int32_t

    kKeySampleBits           = 'sbit', // int32_t (audio sample bit-width)
    kKeyPcmFormat            = 'pfmt', //int32_t (pcm format)
    kKeyMinBlkSize           = 'mibs', //int32_t
    kKeyMaxBlkSize           = 'mabs', //int32_t
    kKeyMinFrmSize           = 'mifs', //int32_t
    kKeyMaxFrmSize           = 'mafs', //int32_t
    kKeyMd5Sum               = 'md5s', //cstring

    kKeyBatchSize            = 'btch', //int32_t
    kKeyIsByteMode           = 'bytm', //int32_t
    kKeyUseSetBuffers        = 'setb', //bool (int32_t)
    kKeyExtraFlag            = 'extf', //int32_t
    kKeyIsDRM                = 'idrm', // int32_t (bool)
    kKeyTimestampReorder     = 'tiRe',
};

enum {
    kTypeDivXVer_3_11,
    kTypeDivXVer_4,
    kTypeDivXVer_5,
    kTypeDivXVer_6,
};
enum {
    kTypeWMA,
    kTypeWMAPro,
    kTypeWMALossLess,
};

//This enum should be keep in sync with "enum Flags" in MediaExtractor.h in AOSP,
//Value should reflect as last entry in the enum
enum {
    CAN_SEEK_TO_ZERO   = 16, // the "previous button"
};

enum {
    USE_SET_BUFFERS = 0x1,
    USE_AUDIO_BIG_BUFFERS = 0x2,
};
}  // namespace android

#endif  // QC_META_DATA_H_
