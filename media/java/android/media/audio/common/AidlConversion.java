/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.media.audio.common;

import android.annotation.NonNull;
import android.media.AudioFormat;
import android.media.MediaFormat;
import android.os.Parcel;

import com.android.internal.annotations.VisibleForTesting;

/**
 * This class provides utility functions for converting between
 * the AIDL types defined in 'android.media.audio.common' and:
 *  - SDK (Java API) types from 'android.media';
 *  - legacy native (system/audio.h) types.
 *
 * Methods that convert between AIDL and SDK types are called
 * using the following pattern:
 *
 *  aidl2api_AIDL-type-name_SDK-type-name
 *  api2aidl_SDK-type-name_AIDL-type-name
 *
 * Since the range of the SDK values is generally narrower than
 * the range of AIDL values, when a match can't be found, the
 * conversion function returns a corresponding 'INVALID' value.
 *
 * Methods that convert between AIDL and legacy types are called
 * using the following pattern:
 *
 *  aidl2legacy_AIDL-type-name_native-type-name
 *  legacy2aidl_native-type-name_AIDL-type-name
 *
 * In general, there is a 1:1 mapping between AIDL and framework
 * types, and a failure to convert a value indicates a programming
 * error. Thus, the conversion functions may throw an IllegalArgumentException.
 *
 * @hide
 */
@VisibleForTesting
public class AidlConversion {
    /** Convert from AIDL AudioChannelLayout to legacy audio_channel_mask_t. */
    public static int /*audio_channel_mask_t*/ aidl2legacy_AudioChannelLayout_audio_channel_mask_t(
            @NonNull AudioChannelLayout aidl, boolean isInput) {
        Parcel out = Parcel.obtain();
        aidl.writeToParcel(out, 0);
        out.setDataPosition(0);
        try {
            return aidl2legacy_AudioChannelLayout_Parcel_audio_channel_mask_t(out, isInput);
        } finally {
            out.recycle();
        }
    }

    /** Convert from legacy audio_channel_mask_t to AIDL AudioChannelLayout. */
    public static AudioChannelLayout legacy2aidl_audio_channel_mask_t_AudioChannelLayout(
            int /*audio_channel_mask_t*/ legacy, boolean isInput) {
        Parcel in = legacy2aidl_audio_channel_mask_t_AudioChannelLayout_Parcel(legacy, isInput);
        if (in != null) {
            try {
                return AudioChannelLayout.CREATOR.createFromParcel(in);
            } finally {
                in.recycle();
            }
        }
        throw new IllegalArgumentException("Failed to convert legacy audio "
                + (isInput ? "input" : "output") + " audio_channel_mask_t " + legacy + " value");
    }

    /** Convert from AIDL AudioFormatDescription to legacy audio_format_t. */
    public static int /*audio_format_t*/ aidl2legacy_AudioFormatDescription_audio_format_t(
            @NonNull AudioFormatDescription aidl) {
        Parcel out = Parcel.obtain();
        aidl.writeToParcel(out, 0);
        out.setDataPosition(0);
        try {
            return aidl2legacy_AudioFormatDescription_Parcel_audio_format_t(out);
        } finally {
            out.recycle();
        }
    }

    /** Convert from legacy audio_format_t to AIDL AudioFormatDescription. */
    public static @NonNull AudioFormatDescription legacy2aidl_audio_format_t_AudioFormatDescription(
            int /*audio_format_t*/ legacy) {
        Parcel in = legacy2aidl_audio_format_t_AudioFormatDescription_Parcel(legacy);
        if (in != null) {
            try {
                return AudioFormatDescription.CREATOR.createFromParcel(in);
            } finally {
                in.recycle();
            }
        }
        throw new IllegalArgumentException(
                "Failed to convert legacy audio_format_t value " + legacy);
    }

    /** Convert from AIDL AudioEncapsulationMode to legacy audio_encapsulation_mode_t. */
    public static native int aidl2legacy_AudioEncapsulationMode_audio_encapsulation_mode_t(
            int /*AudioEncapsulationMode.* */ aidl);

    /** Convert from legacy audio_encapsulation_mode_t to AIDL AudioEncapsulationMode. */
    public static native int legacy2aidl_audio_encapsulation_mode_t_AudioEncapsulationMode(
            int /*audio_encapsulation_mode_t*/ legacy);

    /** Convert from AIDL AudioStreamType to legacy audio_stream_type_t. */
    public static native int aidl2legacy_AudioStreamType_audio_stream_type_t(
            int /*AudioStreamType.* */ aidl);

    /** Convert from legacy audio_stream_type_t to AIDL AudioStreamType. */
    public static native int legacy2aidl_audio_stream_type_t_AudioStreamType(
            int /*audio_stream_type_t*/ legacy);

    /** Convert from AIDL AudioUsage to legacy audio_usage_t. */
    public static native int aidl2legacy_AudioUsage_audio_usage_t(int /*AudioUsage.* */ aidl);

    /** Convert from legacy audio_usage_t to AIDL AudioUsage. */
    public static native int legacy2aidl_audio_usage_t_AudioUsage(int /*audio_usage_t*/ legacy);

    /** Convert from a channel bit of AIDL AudioChannelLayout to SDK AudioFormat.CHANNEL_* bit. */
    private static int aidl2api_AudioChannelLayoutBit_AudioFormatChannel(
            int aidlBit, boolean isInput) {
        if (isInput) {
            switch (aidlBit) {
                case AudioChannelLayout.CHANNEL_FRONT_LEFT:
                    return AudioFormat.CHANNEL_IN_LEFT;
                case AudioChannelLayout.CHANNEL_FRONT_RIGHT:
                    return AudioFormat.CHANNEL_IN_RIGHT;
                case AudioChannelLayout.CHANNEL_FRONT_CENTER:
                    return AudioFormat.CHANNEL_IN_CENTER;
                case AudioChannelLayout.CHANNEL_BACK_CENTER:
                    return AudioFormat.CHANNEL_IN_BACK;
                // CHANNEL_IN_*_PROCESSED not supported
                // CHANNEL_IN_PRESSURE not supported
                // CHANNEL_IN_*_AXIS not supported
                // CHANNEL_IN_VOICE_* not supported
                case AudioChannelLayout.CHANNEL_BACK_LEFT:
                    return AudioFormat.CHANNEL_IN_BACK_LEFT;
                case AudioChannelLayout.CHANNEL_BACK_RIGHT:
                    return AudioFormat.CHANNEL_IN_BACK_RIGHT;
                case AudioChannelLayout.CHANNEL_LOW_FREQUENCY:
                    return AudioFormat.CHANNEL_IN_LOW_FREQUENCY;
                case AudioChannelLayout.CHANNEL_TOP_SIDE_LEFT:
                    return AudioFormat.CHANNEL_IN_TOP_LEFT;
                case AudioChannelLayout.CHANNEL_TOP_SIDE_RIGHT:
                    return AudioFormat.CHANNEL_IN_TOP_RIGHT;
                default:
                    return AudioFormat.CHANNEL_INVALID;
            }
        } else {
            switch (aidlBit) {
                case AudioChannelLayout.CHANNEL_FRONT_LEFT:
                    return AudioFormat.CHANNEL_OUT_FRONT_LEFT;
                case AudioChannelLayout.CHANNEL_FRONT_RIGHT:
                    return AudioFormat.CHANNEL_OUT_FRONT_RIGHT;
                case AudioChannelLayout.CHANNEL_FRONT_CENTER:
                    return AudioFormat.CHANNEL_OUT_FRONT_CENTER;
                case AudioChannelLayout.CHANNEL_LOW_FREQUENCY:
                    return AudioFormat.CHANNEL_OUT_LOW_FREQUENCY;
                case AudioChannelLayout.CHANNEL_BACK_LEFT:
                    return AudioFormat.CHANNEL_OUT_BACK_LEFT;
                case AudioChannelLayout.CHANNEL_BACK_RIGHT:
                    return AudioFormat.CHANNEL_OUT_BACK_RIGHT;
                case AudioChannelLayout.CHANNEL_FRONT_LEFT_OF_CENTER:
                    return AudioFormat.CHANNEL_OUT_FRONT_LEFT_OF_CENTER;
                case AudioChannelLayout.CHANNEL_FRONT_RIGHT_OF_CENTER:
                    return AudioFormat.CHANNEL_OUT_FRONT_RIGHT_OF_CENTER;
                case AudioChannelLayout.CHANNEL_BACK_CENTER:
                    return AudioFormat.CHANNEL_OUT_BACK_CENTER;
                case AudioChannelLayout.CHANNEL_SIDE_LEFT:
                    return AudioFormat.CHANNEL_OUT_SIDE_LEFT;
                case AudioChannelLayout.CHANNEL_SIDE_RIGHT:
                    return AudioFormat.CHANNEL_OUT_SIDE_RIGHT;
                case AudioChannelLayout.CHANNEL_TOP_CENTER:
                    return AudioFormat.CHANNEL_OUT_TOP_CENTER;
                case AudioChannelLayout.CHANNEL_TOP_FRONT_LEFT:
                    return AudioFormat.CHANNEL_OUT_TOP_FRONT_LEFT;
                case AudioChannelLayout.CHANNEL_TOP_FRONT_CENTER:
                    return AudioFormat.CHANNEL_OUT_TOP_FRONT_CENTER;
                case AudioChannelLayout.CHANNEL_TOP_FRONT_RIGHT:
                    return AudioFormat.CHANNEL_OUT_TOP_FRONT_RIGHT;
                case AudioChannelLayout.CHANNEL_TOP_BACK_LEFT:
                    return AudioFormat.CHANNEL_OUT_TOP_BACK_LEFT;
                case AudioChannelLayout.CHANNEL_TOP_BACK_CENTER:
                    return AudioFormat.CHANNEL_OUT_TOP_BACK_CENTER;
                case AudioChannelLayout.CHANNEL_TOP_BACK_RIGHT:
                    return AudioFormat.CHANNEL_OUT_TOP_BACK_RIGHT;
                case AudioChannelLayout.CHANNEL_TOP_SIDE_LEFT:
                    return AudioFormat.CHANNEL_OUT_TOP_SIDE_LEFT;
                case AudioChannelLayout.CHANNEL_TOP_SIDE_RIGHT:
                    return AudioFormat.CHANNEL_OUT_TOP_SIDE_RIGHT;
                case AudioChannelLayout.CHANNEL_BOTTOM_FRONT_LEFT:
                    return AudioFormat.CHANNEL_OUT_BOTTOM_FRONT_LEFT;
                case AudioChannelLayout.CHANNEL_BOTTOM_FRONT_CENTER:
                    return AudioFormat.CHANNEL_OUT_BOTTOM_FRONT_CENTER;
                case AudioChannelLayout.CHANNEL_BOTTOM_FRONT_RIGHT:
                    return AudioFormat.CHANNEL_OUT_BOTTOM_FRONT_RIGHT;
                case AudioChannelLayout.CHANNEL_LOW_FREQUENCY_2:
                    return AudioFormat.CHANNEL_OUT_LOW_FREQUENCY_2;
                case AudioChannelLayout.CHANNEL_FRONT_WIDE_LEFT:
                    return AudioFormat.CHANNEL_OUT_FRONT_WIDE_LEFT;
                case AudioChannelLayout.CHANNEL_FRONT_WIDE_RIGHT:
                    return AudioFormat.CHANNEL_OUT_FRONT_WIDE_RIGHT;
                case AudioChannelLayout.CHANNEL_HAPTIC_A:
                    return AudioFormat.CHANNEL_OUT_HAPTIC_A;
                case AudioChannelLayout.CHANNEL_HAPTIC_B:
                    return AudioFormat.CHANNEL_OUT_HAPTIC_B;
                default:
                    return AudioFormat.CHANNEL_INVALID;
            }
        }
    }

    /**
     * Convert from a channel bitmask of AIDL AudioChannelLayout to
     * SDK AudioFormat.CHANNEL_* bitmask.
     */
    private static int aidl2api_AudioChannelLayoutBitMask_AudioFormatChannelMask(
            int aidlBitMask, boolean isInput) {
        int apiMask = 0;
        for (int bit = 1 << 31; bit != 0; bit >>>= 1) {
            if ((aidlBitMask & bit) == bit) {
                int apiBit = aidl2api_AudioChannelLayoutBit_AudioFormatChannel(bit, isInput);
                if (apiBit != AudioFormat.CHANNEL_INVALID) {
                    apiMask |= apiBit;
                    aidlBitMask &= ~bit;
                    if (aidlBitMask == 0) {
                        return apiMask;
                    }
                } else {
                    break;
                }
            }
        }
        return AudioFormat.CHANNEL_INVALID;
    }

    /** Convert from AIDL AudioChannelLayout to SDK AudioFormat.CHANNEL_*. */
    public static int aidl2api_AudioChannelLayout_AudioFormatChannelMask(
            @NonNull AudioChannelLayout aidlMask, boolean isInput) {
        switch (aidlMask.getTag()) {
            case AudioChannelLayout.none:
                return isInput ? AudioFormat.CHANNEL_IN_DEFAULT : AudioFormat.CHANNEL_OUT_DEFAULT;
            case AudioChannelLayout.invalid:
                return AudioFormat.CHANNEL_INVALID;
            case AudioChannelLayout.indexMask:
                // Note that for a proper building of SDK AudioFormat one must
                // call either 'setChannelMask' or 'setChannelIndexMask' depending
                // on the variant of AudioChannelLayout. The integer representations
                // of positional and indexed channel masks are indistinguishable in
                // the SDK.
                return aidlMask.getIndexMask();
            case AudioChannelLayout.layoutMask:
                if (isInput) {
                    switch (aidlMask.getLayoutMask()) {
                        case AudioChannelLayout.LAYOUT_MONO:
                            return AudioFormat.CHANNEL_IN_MONO;
                        case AudioChannelLayout.LAYOUT_STEREO:
                            return AudioFormat.CHANNEL_IN_STEREO;
                        case AudioChannelLayout.LAYOUT_FRONT_BACK:
                            return AudioFormat.CHANNEL_IN_FRONT_BACK;
                        case AudioChannelLayout.LAYOUT_2POINT0POINT2:
                            return AudioFormat.CHANNEL_IN_2POINT0POINT2;
                        case AudioChannelLayout.LAYOUT_2POINT1POINT2:
                            return AudioFormat.CHANNEL_IN_2POINT1POINT2;
                        case AudioChannelLayout.LAYOUT_3POINT0POINT2:
                            return AudioFormat.CHANNEL_IN_3POINT0POINT2;
                        case AudioChannelLayout.LAYOUT_3POINT1POINT2:
                            return AudioFormat.CHANNEL_IN_3POINT1POINT2;
                        case AudioChannelLayout.LAYOUT_5POINT1:
                            return AudioFormat.CHANNEL_IN_5POINT1;
                        default: // fall through
                    }
                } else {
                    switch (aidlMask.getLayoutMask()) {
                        case AudioChannelLayout.LAYOUT_MONO:
                            return AudioFormat.CHANNEL_OUT_MONO;
                        case AudioChannelLayout.LAYOUT_STEREO:
                            return AudioFormat.CHANNEL_OUT_STEREO;
                        case AudioChannelLayout.LAYOUT_2POINT1:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_LOW_FREQUENCY;
                        case AudioChannelLayout.LAYOUT_TRI:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_FRONT_CENTER;
                        case AudioChannelLayout.LAYOUT_TRI_BACK:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_BACK_CENTER;
                        case AudioChannelLayout.LAYOUT_3POINT1:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_FRONT_CENTER
                                    | AudioFormat.CHANNEL_OUT_LOW_FREQUENCY;
                        case AudioChannelLayout.LAYOUT_2POINT0POINT2:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_TOP_SIDE_LEFT
                                    | AudioFormat.CHANNEL_OUT_TOP_SIDE_RIGHT;
                        case AudioChannelLayout.LAYOUT_2POINT1POINT2:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_TOP_SIDE_LEFT
                                    | AudioFormat.CHANNEL_OUT_TOP_SIDE_RIGHT
                                    | AudioFormat.CHANNEL_OUT_LOW_FREQUENCY;
                        case AudioChannelLayout.LAYOUT_3POINT0POINT2:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_FRONT_CENTER
                                    | AudioFormat.CHANNEL_OUT_TOP_SIDE_LEFT
                                    | AudioFormat.CHANNEL_OUT_TOP_SIDE_RIGHT;
                        case AudioChannelLayout.LAYOUT_3POINT1POINT2:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_FRONT_CENTER
                                    | AudioFormat.CHANNEL_OUT_TOP_SIDE_LEFT
                                    | AudioFormat.CHANNEL_OUT_TOP_SIDE_RIGHT
                                    | AudioFormat.CHANNEL_OUT_LOW_FREQUENCY;
                        case AudioChannelLayout.LAYOUT_QUAD:
                            return AudioFormat.CHANNEL_OUT_QUAD;
                        case AudioChannelLayout.LAYOUT_QUAD_SIDE:
                            return AudioFormat.CHANNEL_OUT_QUAD_SIDE;
                        case AudioChannelLayout.LAYOUT_SURROUND:
                            return AudioFormat.CHANNEL_OUT_SURROUND;
                        case AudioChannelLayout.LAYOUT_PENTA:
                            return AudioFormat.CHANNEL_OUT_QUAD
                                    | AudioFormat.CHANNEL_OUT_FRONT_CENTER;
                        case AudioChannelLayout.LAYOUT_5POINT1:
                            return AudioFormat.CHANNEL_OUT_5POINT1;
                        case AudioChannelLayout.LAYOUT_5POINT1_SIDE:
                            return AudioFormat.CHANNEL_OUT_5POINT1_SIDE;
                        case AudioChannelLayout.LAYOUT_5POINT1POINT2:
                            return AudioFormat.CHANNEL_OUT_5POINT1POINT2;
                        case AudioChannelLayout.LAYOUT_5POINT1POINT4:
                            return AudioFormat.CHANNEL_OUT_5POINT1POINT4;
                        case AudioChannelLayout.LAYOUT_6POINT1:
                            return AudioFormat.CHANNEL_OUT_FRONT_LEFT
                                    | AudioFormat.CHANNEL_OUT_FRONT_RIGHT
                                    | AudioFormat.CHANNEL_OUT_FRONT_CENTER
                                    | AudioFormat.CHANNEL_OUT_LOW_FREQUENCY
                                    | AudioFormat.CHANNEL_OUT_BACK_LEFT
                                    | AudioFormat.CHANNEL_OUT_BACK_RIGHT
                                    | AudioFormat.CHANNEL_OUT_BACK_CENTER;
                        case AudioChannelLayout.LAYOUT_7POINT1:
                            return AudioFormat.CHANNEL_OUT_7POINT1_SURROUND;
                        case AudioChannelLayout.LAYOUT_7POINT1POINT2:
                            return AudioFormat.CHANNEL_OUT_7POINT1POINT2;
                        case AudioChannelLayout.LAYOUT_7POINT1POINT4:
                            return AudioFormat.CHANNEL_OUT_7POINT1POINT4;
                        case AudioChannelLayout.LAYOUT_9POINT1POINT4:
                            return AudioFormat.CHANNEL_OUT_9POINT1POINT4;
                        case AudioChannelLayout.LAYOUT_9POINT1POINT6:
                            return AudioFormat.CHANNEL_OUT_9POINT1POINT6;
                        case AudioChannelLayout.LAYOUT_13POINT_360RA:
                            return AudioFormat.CHANNEL_OUT_13POINT_360RA;
                        case AudioChannelLayout.LAYOUT_22POINT2:
                            return AudioFormat.CHANNEL_OUT_22POINT2;
                        case AudioChannelLayout.LAYOUT_MONO_HAPTIC_A:
                            return AudioFormat.CHANNEL_OUT_MONO
                                    | AudioFormat.CHANNEL_OUT_HAPTIC_A;
                        case AudioChannelLayout.LAYOUT_STEREO_HAPTIC_A:
                            return AudioFormat.CHANNEL_OUT_STEREO
                                    | AudioFormat.CHANNEL_OUT_HAPTIC_A;
                        case AudioChannelLayout.LAYOUT_HAPTIC_AB:
                            return AudioFormat.CHANNEL_OUT_HAPTIC_A
                                    | AudioFormat.CHANNEL_OUT_HAPTIC_B;
                        case AudioChannelLayout.LAYOUT_MONO_HAPTIC_AB:
                            return AudioFormat.CHANNEL_OUT_MONO
                                    | AudioFormat.CHANNEL_OUT_HAPTIC_A
                                    | AudioFormat.CHANNEL_OUT_HAPTIC_B;
                        case AudioChannelLayout.LAYOUT_STEREO_HAPTIC_AB:
                            return AudioFormat.CHANNEL_OUT_STEREO
                                    | AudioFormat.CHANNEL_OUT_HAPTIC_A
                                    | AudioFormat.CHANNEL_OUT_HAPTIC_B;
                        case AudioChannelLayout.LAYOUT_FRONT_BACK:
                            return AudioFormat.CHANNEL_OUT_FRONT_CENTER
                                    | AudioFormat.CHANNEL_OUT_BACK_CENTER;
                        default: // fall through
                    }
                }
                // If a match for a predefined layout wasn't found, make a custom one from bits.
                return aidl2api_AudioChannelLayoutBitMask_AudioFormatChannelMask(
                        aidlMask.getLayoutMask(), isInput);
            case AudioChannelLayout.voiceMask:
                if (isInput) {
                    switch (aidlMask.getVoiceMask()) {
                        // AudioFormat input masks match legacy native masks directly,
                        // thus we add AUDIO_CHANNEL_IN_VOICE_UPLINK_MONO here.
                        case AudioChannelLayout.VOICE_UPLINK_MONO:
                            return AudioFormat.CHANNEL_IN_VOICE_UPLINK
                                    | AudioFormat.CHANNEL_IN_MONO;
                        case AudioChannelLayout.VOICE_DNLINK_MONO:
                            return AudioFormat.CHANNEL_IN_VOICE_DNLINK
                                    | AudioFormat.CHANNEL_IN_MONO;
                        case AudioChannelLayout.VOICE_CALL_MONO:
                            return AudioFormat.CHANNEL_IN_VOICE_UPLINK
                                    | AudioFormat.CHANNEL_IN_VOICE_DNLINK
                                    | AudioFormat.CHANNEL_IN_MONO;
                    }
                }
                return AudioFormat.CHANNEL_INVALID;
            default:
                return AudioFormat.CHANNEL_INVALID;
        }
    }

    /** Convert from AIDL AudioConfig to SDK AudioFormat. */
    public static @NonNull AudioFormat aidl2api_AudioConfig_AudioFormat(
            @NonNull AudioConfig aidl, boolean isInput) {
        // Only information from the encapsulated AudioConfigBase is used.
        return aidl2api_AudioConfigBase_AudioFormat(aidl.base, isInput);
    }

    /** Convert from AIDL AudioConfigBase to SDK AudioFormat. */
    public static @NonNull AudioFormat aidl2api_AudioConfigBase_AudioFormat(
            @NonNull AudioConfigBase aidl, boolean isInput) {
        AudioFormat.Builder apiBuilder = new AudioFormat.Builder();
        apiBuilder.setSampleRate(aidl.sampleRate);
        if (aidl.channelMask.getTag() != AudioChannelLayout.indexMask) {
            apiBuilder.setChannelMask(aidl2api_AudioChannelLayout_AudioFormatChannelMask(
                            aidl.channelMask, isInput));
        } else {
            apiBuilder.setChannelIndexMask(aidl2api_AudioChannelLayout_AudioFormatChannelMask(
                            aidl.channelMask, isInput));
        }
        apiBuilder.setEncoding(aidl2api_AudioFormat_AudioFormatEncoding(aidl.format));
        return apiBuilder.build();
    }

    /** Convert from AIDL AudioFormat to SDK AudioFormat.ENCODING_*. */
    public static int aidl2api_AudioFormat_AudioFormatEncoding(
            @NonNull AudioFormatDescription aidl) {
        switch (aidl.type) {
            case AudioFormatType.PCM:
                switch (aidl.pcm) {
                    case PcmType.UINT_8_BIT:
                        return AudioFormat.ENCODING_PCM_8BIT;
                    case PcmType.INT_16_BIT:
                        return AudioFormat.ENCODING_PCM_16BIT;
                    case PcmType.INT_32_BIT:
                        return AudioFormat.ENCODING_PCM_32BIT;
                    case PcmType.FIXED_Q_8_24:
                    case PcmType.FLOAT_32_BIT:
                        return AudioFormat.ENCODING_PCM_FLOAT;
                    case PcmType.INT_24_BIT:
                        return AudioFormat.ENCODING_PCM_24BIT_PACKED;
                    default:
                        return AudioFormat.ENCODING_INVALID;
                }
            case AudioFormatType.NON_PCM: // same as DEFAULT
                if (aidl.encoding != null && !aidl.encoding.isEmpty()) {
                    if (MediaFormat.MIMETYPE_AUDIO_AC3.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_AC3;
                    } else if (MediaFormat.MIMETYPE_AUDIO_EAC3.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_E_AC3;
                    } else if (MediaFormat.MIMETYPE_AUDIO_DTS.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_DTS;
                    } else if (MediaFormat.MIMETYPE_AUDIO_DTS_HD.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_DTS_HD;
                    } else if (MediaFormat.MIMETYPE_AUDIO_MPEG.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_MP3;
                    } else if (MediaFormat.MIMETYPE_AUDIO_AAC_LC.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_AAC_LC;
                    } else if (MediaFormat.MIMETYPE_AUDIO_AAC_HE_V1.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_AAC_HE_V1;
                    } else if (MediaFormat.MIMETYPE_AUDIO_AAC_HE_V2.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_AAC_HE_V2;
                    } else if (MediaFormat.MIMETYPE_AUDIO_IEC61937.equals(aidl.encoding)
                            && aidl.pcm == PcmType.INT_16_BIT) {
                        return AudioFormat.ENCODING_IEC61937;
                    } else if (MediaFormat.MIMETYPE_AUDIO_DOLBY_TRUEHD.equals(
                                    aidl.encoding)) {
                        return AudioFormat.ENCODING_DOLBY_TRUEHD;
                    } else if (MediaFormat.MIMETYPE_AUDIO_AAC_ELD.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_AAC_ELD;
                    } else if (MediaFormat.MIMETYPE_AUDIO_AAC_XHE.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_AAC_XHE;
                    } else if (MediaFormat.MIMETYPE_AUDIO_AC4.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_AC4;
                    } else if (MediaFormat.MIMETYPE_AUDIO_EAC3_JOC.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_E_AC3_JOC;
                    } else if (MediaFormat.MIMETYPE_AUDIO_DOLBY_MAT.equals(aidl.encoding)
                            || aidl.encoding.startsWith(
                                    MediaFormat.MIMETYPE_AUDIO_DOLBY_MAT + ".")) {
                        return AudioFormat.ENCODING_DOLBY_MAT;
                    } else if (MediaFormat.MIMETYPE_AUDIO_OPUS.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_OPUS;
                    } else if (MediaFormat.MIMETYPE_AUDIO_MPEGH_BL_L3.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_MPEGH_BL_L3;
                    } else if (MediaFormat.MIMETYPE_AUDIO_MPEGH_BL_L4.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_MPEGH_BL_L4;
                    } else if (MediaFormat.MIMETYPE_AUDIO_MPEGH_LC_L3.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_MPEGH_LC_L3;
                    } else if (MediaFormat.MIMETYPE_AUDIO_MPEGH_LC_L4.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_MPEGH_LC_L4;
                    } else if (MediaFormat.MIMETYPE_AUDIO_DTS_UHD.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_DTS_UHD;
                    } else if (MediaFormat.MIMETYPE_AUDIO_DRA.equals(aidl.encoding)) {
                        return AudioFormat.ENCODING_DRA;
                    } else {
                        return AudioFormat.ENCODING_INVALID;
                    }
                } else {
                    return AudioFormat.ENCODING_DEFAULT;
                }
            case AudioFormatType.SYS_RESERVED_INVALID:
            default:
                return AudioFormat.ENCODING_INVALID;
        }
    }

    private static native int aidl2legacy_AudioChannelLayout_Parcel_audio_channel_mask_t(
            Parcel aidl, boolean isInput);
    private static native Parcel legacy2aidl_audio_channel_mask_t_AudioChannelLayout_Parcel(
            int legacy, boolean isInput);
    private static native int aidl2legacy_AudioFormatDescription_Parcel_audio_format_t(
            Parcel aidl);
    private static native Parcel legacy2aidl_audio_format_t_AudioFormatDescription_Parcel(
            int legacy);
}
