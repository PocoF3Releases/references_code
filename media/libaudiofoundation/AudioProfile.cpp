/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <set>

#define LOG_TAG "AudioProfile"
//#define LOG_NDEBUG 0

#include <android-base/stringprintf.h>
#include <media/AudioContainers.h>
#include <media/AudioProfile.h>
#include <media/TypeConverter.h>
#include <utils/Errors.h>

namespace android {

bool operator == (const AudioProfile &left, const AudioProfile &right)
{
    return (left.getFormat() == right.getFormat()) &&
            (left.getChannels() == right.getChannels()) &&
            (left.getSampleRates() == right.getSampleRates());
}

// static
sp<AudioProfile> AudioProfile::createFullDynamic(audio_format_t dynamicFormat)
{
    AudioProfile* dynamicProfile = new AudioProfile(dynamicFormat,
            ChannelMaskSet(), SampleRateSet());
    dynamicProfile->setDynamicFormat(true);
    dynamicProfile->setDynamicChannels(true);
    dynamicProfile->setDynamicRate(true);
    return dynamicProfile;
}

AudioProfile::AudioProfile(audio_format_t format,
                           audio_channel_mask_t channelMasks,
                           uint32_t samplingRate) :
        mName(""),
        mFormat(format)
{
    mChannelMasks.insert(channelMasks);
    mSamplingRates.insert(samplingRate);
}

AudioProfile::AudioProfile(audio_format_t format,
                           const ChannelMaskSet &channelMasks,
                           const SampleRateSet &samplingRateCollection) :
        AudioProfile(format, channelMasks, samplingRateCollection,
                     AUDIO_ENCAPSULATION_TYPE_NONE) {}

AudioProfile::AudioProfile(audio_format_t format,
                           const ChannelMaskSet &channelMasks,
                           const SampleRateSet &samplingRateCollection,
                           audio_encapsulation_type_t encapsulationType) :
        mName(""),
        mFormat(format),
        mChannelMasks(channelMasks),
        mSamplingRates(samplingRateCollection),
        mEncapsulationType(encapsulationType) {}

void AudioProfile::setChannels(const ChannelMaskSet &channelMasks)
{
    if (mIsDynamicChannels) {
        mChannelMasks = channelMasks;
    }
}

void AudioProfile::setSampleRates(const SampleRateSet &sampleRates)
{
    if (mIsDynamicRate) {
        mSamplingRates = sampleRates;
    }
}

void AudioProfile::clear()
{
    if (mIsDynamicChannels) {
        mChannelMasks.clear();
    }
    if (mIsDynamicRate) {
        mSamplingRates.clear();
    }
}

void AudioProfile::dump(std::string *dst, int spaces) const
{
    dst->append(base::StringPrintf("%s%s%s\n", mIsDynamicFormat ? "[dynamic format]" : "",
             mIsDynamicChannels ? "[dynamic channels]" : "",
             mIsDynamicRate ? "[dynamic rates]" : ""));
    if (mName.length() != 0) {
        dst->append(base::StringPrintf("%*s- name: %s\n", spaces, "", mName.c_str()));
    }
    std::string formatLiteral;
    if (FormatConverter::toString(mFormat, formatLiteral)) {
        dst->append(base::StringPrintf("%*s- format: %s\n", spaces, "", formatLiteral.c_str()));
    }
    if (!mSamplingRates.empty()) {
        dst->append(base::StringPrintf("%*s- sampling rates:", spaces, ""));
        for (auto it = mSamplingRates.begin(); it != mSamplingRates.end();) {
            dst->append(base::StringPrintf("%d", *it));
            dst->append(++it == mSamplingRates.end() ? "" : ", ");
        }
        dst->append("\n");
    }

    if (!mChannelMasks.empty()) {
        dst->append(base::StringPrintf("%*s- channel masks:", spaces, ""));
        for (auto it = mChannelMasks.begin(); it != mChannelMasks.end();) {
            dst->append(base::StringPrintf("0x%04x", *it));
            dst->append(++it == mChannelMasks.end() ? "" : ", ");
        }
        dst->append("\n");
    }

    dst->append(base::StringPrintf(
            "%*s- encapsulation type: %#x\n", spaces, "", mEncapsulationType));
}

bool AudioProfile::equals(const sp<AudioProfile>& other) const
{
    return other != nullptr &&
           mName.compare(other->mName) == 0 &&
           mFormat == other->getFormat() &&
           mChannelMasks == other->getChannels() &&
           mSamplingRates == other->getSampleRates() &&
           mIsDynamicFormat == other->isDynamicFormat() &&
           mIsDynamicChannels == other->isDynamicChannels() &&
           mIsDynamicRate == other->isDynamicRate() &&
           mEncapsulationType == other->getEncapsulationType();
}

AudioProfile& AudioProfile::operator=(const AudioProfile& other) {
    mName = other.mName;
    mFormat = other.mFormat;
    mChannelMasks = other.mChannelMasks;
    mSamplingRates = other.mSamplingRates;
    mEncapsulationType = other.mEncapsulationType;
    mIsDynamicFormat = other.mIsDynamicFormat;
    mIsDynamicChannels = other.mIsDynamicChannels;
    mIsDynamicRate = other.mIsDynamicRate;
    return *this;
}

ConversionResult<media::AudioProfile>
AudioProfile::toParcelable(bool isInput) const {
    media::AudioProfile parcelable;
    parcelable.name = mName;
    parcelable.format = VALUE_OR_RETURN(legacy2aidl_audio_format_t_AudioFormatDescription(mFormat));
    parcelable.channelMasks = VALUE_OR_RETURN(
            convertContainer<std::vector<media::AudioChannelLayout>>(
                    mChannelMasks,
                    [isInput](audio_channel_mask_t m) {
                        return legacy2aidl_audio_channel_mask_t_AudioChannelLayout(m, isInput);
                    }));
    parcelable.samplingRates = VALUE_OR_RETURN(
            convertContainer<std::vector<int32_t>>(mSamplingRates,
                                                   convertIntegral<int32_t, uint32_t>));
    parcelable.isDynamicFormat = mIsDynamicFormat;
    parcelable.isDynamicChannels = mIsDynamicChannels;
    parcelable.isDynamicRate = mIsDynamicRate;
    parcelable.encapsulationType = VALUE_OR_RETURN(
            legacy2aidl_audio_encapsulation_type_t_AudioEncapsulationType(mEncapsulationType));
    return parcelable;
}

ConversionResult<sp<AudioProfile>>
AudioProfile::fromParcelable(const media::AudioProfile& parcelable, bool isInput) {
    sp<AudioProfile> legacy = new AudioProfile();
    legacy->mName = parcelable.name;
    legacy->mFormat = VALUE_OR_RETURN(
            aidl2legacy_AudioFormatDescription_audio_format_t(parcelable.format));
    legacy->mChannelMasks = VALUE_OR_RETURN(
            convertContainer<ChannelMaskSet>(parcelable.channelMasks,
                    [isInput](const media::AudioChannelLayout& l) {
                        return aidl2legacy_AudioChannelLayout_audio_channel_mask_t(l, isInput);
                    }));
    legacy->mSamplingRates = VALUE_OR_RETURN(
            convertContainer<SampleRateSet>(parcelable.samplingRates,
                                            convertIntegral<uint32_t, int32_t>));
    legacy->mIsDynamicFormat = parcelable.isDynamicFormat;
    legacy->mIsDynamicChannels = parcelable.isDynamicChannels;
    legacy->mIsDynamicRate = parcelable.isDynamicRate;
    legacy->mEncapsulationType = VALUE_OR_RETURN(
            aidl2legacy_AudioEncapsulationType_audio_encapsulation_type_t(
                    parcelable.encapsulationType));
    return legacy;
}

ConversionResult<sp<AudioProfile>>
aidl2legacy_AudioProfile(const media::AudioProfile& aidl, bool isInput) {
    return AudioProfile::fromParcelable(aidl, isInput);
}

ConversionResult<media::AudioProfile>
legacy2aidl_AudioProfile(const sp<AudioProfile>& legacy, bool isInput) {
    return legacy->toParcelable(isInput);
}

ssize_t AudioProfileVector::add(const sp<AudioProfile> &profile)
{
    ssize_t index = size();
    push_back(profile);
    return index;
}

void AudioProfileVector::clearProfiles()
{
    for (auto it = begin(); it != end();) {
        if ((*it)->isDynamicFormat() && (*it)->hasValidFormat()) {
            it = erase(it);
        } else {
            (*it)->clear();
            ++it;
        }
    }
}

sp<AudioProfile> AudioProfileVector::getFirstValidProfile() const
{
    for (const auto &profile : *this) {
        if (profile->isValid()) {
            return profile;
        }
    }
    return nullptr;
}

sp<AudioProfile> AudioProfileVector::getFirstValidProfileFor(audio_format_t format) const
{
    for (const auto &profile : *this) {
        if (profile->isValid() && profile->getFormat() == format) {
            return profile;
        }
    }
    return nullptr;
}

FormatVector AudioProfileVector::getSupportedFormats() const
{
    FormatVector supportedFormats;
    for (const auto &profile : *this) {
        if (profile->hasValidFormat()) {
            supportedFormats.push_back(profile->getFormat());
        }
    }
    return supportedFormats;
}

bool AudioProfileVector::hasDynamicChannelsFor(audio_format_t format) const
{
    for (const auto &profile : *this) {
        if (profile->getFormat() == format && profile->isDynamicChannels()) {
            return true;
        }
    }
    return false;
}

bool AudioProfileVector::hasDynamicFormat() const
{
    for (const auto &profile : *this) {
        if (profile->isDynamicFormat()) {
            return true;
        }
    }
    return false;
}

bool AudioProfileVector::hasDynamicProfile() const
{
    for (const auto &profile : *this) {
        if (profile->isDynamic()) {
            return true;
        }
    }
    return false;
}

bool AudioProfileVector::hasDynamicRateFor(audio_format_t format) const
{
    for (const auto &profile : *this) {
        if (profile->getFormat() == format && profile->isDynamicRate()) {
            return true;
        }
    }
    return false;
}

bool AudioProfileVector::contains(const sp<AudioProfile>& profile) const
{
    for (const auto& audioProfile : *this) {
        if (audioProfile->equals(profile)) {
            return true;
        }
    }
    return false;
}

void AudioProfileVector::dump(std::string *dst, int spaces) const
{
    dst->append(base::StringPrintf("%*s- Profiles:\n", spaces, ""));
    for (size_t i = 0; i < size(); i++) {
        dst->append(base::StringPrintf("%*sProfile %zu:", spaces + 4, "", i));
        std::string profileStr;
        at(i)->dump(&profileStr, spaces + 8);
        dst->append(profileStr);
    }
}

bool AudioProfileVector::equals(const AudioProfileVector& other) const
{
    return std::equal(begin(), end(), other.begin(), other.end(),
                      [](const sp<AudioProfile>& left, const sp<AudioProfile>& right) {
                          return left->equals(right);
                      });
}

ConversionResult<AudioProfileVector>
aidl2legacy_AudioProfileVector(const std::vector<media::AudioProfile>& aidl, bool isInput) {
    return convertContainer<AudioProfileVector>(aidl,
            [isInput](const media::AudioProfile& p) {
                return aidl2legacy_AudioProfile(p, isInput);
            });
}

ConversionResult<std::vector<media::AudioProfile>>
legacy2aidl_AudioProfileVector(const AudioProfileVector& legacy, bool isInput) {
    return convertContainer<std::vector<media::AudioProfile>>(legacy,
            [isInput](const sp<AudioProfile>& p) {
                return legacy2aidl_AudioProfile(p, isInput);
            });
}

AudioProfileVector intersectAudioProfiles(const AudioProfileVector& profiles1,
                                          const AudioProfileVector& profiles2)
{
    std::map<audio_format_t, std::pair<ChannelMaskSet, SampleRateSet>> infos2;
    for (const auto& profile : profiles2) {
        infos2.emplace(profile->getFormat(),
                std::make_pair(profile->getChannels(), profile->getSampleRates()));
    }
    AudioProfileVector profiles;
    for (const auto& profile : profiles1) {
        const auto it = infos2.find(profile->getFormat());
        if (it == infos2.end()) {
            continue;
        }
        ChannelMaskSet channelMasks = SetIntersection(profile->getChannels(), it->second.first);
        if (channelMasks.empty()) {
            continue;
        }
        SampleRateSet sampleRates = SetIntersection(profile->getSampleRates(), it->second.second);
        if (sampleRates.empty()) {
            continue;
        }
        profiles.push_back(new AudioProfile(profile->getFormat(), channelMasks, sampleRates));
    }
    return profiles;
}

} // namespace android
