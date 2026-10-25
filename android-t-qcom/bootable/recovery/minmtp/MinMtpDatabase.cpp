/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include <utils/Log.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <map>
#include <libgen.h>
#include <cutils/properties.h>
#include <android-base/logging.h>

#include "MinMtpDatabase.h"
#include "MtpStorage.h"
#include "MtpDataPacket.h"
#include "MtpObjectInfo.h"
#include "MtpProperty.h"
#include "MtpDebug.h"
#include "MtpStringBuffer.h"
#include "MtpUtils.h"
#include "mtp.h"

MinMtpDatabase::MinMtpDatabase(MtpStorageList * list)
{
    current_handle = 0;
    lstorage = list;
}

MinMtpDatabase::~MinMtpDatabase() {
}

const char *MinMtpDatabase::get_file_path() {
    if(objectNameMap.size() > 0) {
        return objectNameMap[current_handle].c_str();
    }
    return NULL;
}

int MinMtpDatabase::FILE_PROPERTIES[9] = {
    // NOTE must match beginning of AUDIO_PROPERTIES, VIDEO_PROPERTIES
    // and IMAGE_PROPERTIES below
    MTP_PROPERTY_STORAGE_ID,
    MTP_PROPERTY_OBJECT_FORMAT,
    MTP_PROPERTY_PROTECTION_STATUS,
    MTP_PROPERTY_OBJECT_SIZE,
    MTP_PROPERTY_OBJECT_FILE_NAME,
//    MTP_PROPERTY_DATE_MODIFIED,
    MTP_PROPERTY_PARENT_OBJECT,
    MTP_PROPERTY_PERSISTENT_UID,
    MTP_PROPERTY_NAME,
    MTP_PROPERTY_DATE_ADDED
};
int MinMtpDatabase::AUDIO_PROPERTIES[19] = {
    // NOTE must match FILE_PROPERTIES above
    MTP_PROPERTY_STORAGE_ID,
    MTP_PROPERTY_OBJECT_FORMAT,
    MTP_PROPERTY_PROTECTION_STATUS,
    MTP_PROPERTY_OBJECT_SIZE,
    MTP_PROPERTY_OBJECT_FILE_NAME,
    MTP_PROPERTY_DATE_MODIFIED,
    MTP_PROPERTY_PARENT_OBJECT,
    MTP_PROPERTY_PERSISTENT_UID,
    MTP_PROPERTY_NAME,
    MTP_PROPERTY_DISPLAY_NAME,
    MTP_PROPERTY_DATE_ADDED,

    // audio specific properties
    MTP_PROPERTY_ARTIST,
    MTP_PROPERTY_ALBUM_NAME,
    MTP_PROPERTY_ALBUM_ARTIST,
    MTP_PROPERTY_TRACK,
    MTP_PROPERTY_ORIGINAL_RELEASE_DATE,
    MTP_PROPERTY_DURATION,
    MTP_PROPERTY_GENRE,
    MTP_PROPERTY_COMPOSER
};

int MinMtpDatabase::VIDEO_PROPERTIES[15] = {
    // NOTE must match FILE_PROPERTIES above
    MTP_PROPERTY_STORAGE_ID,
    MTP_PROPERTY_OBJECT_FORMAT,
    MTP_PROPERTY_PROTECTION_STATUS,
    MTP_PROPERTY_OBJECT_SIZE,
    MTP_PROPERTY_OBJECT_FILE_NAME,
    MTP_PROPERTY_DATE_MODIFIED,
    MTP_PROPERTY_PARENT_OBJECT,
    MTP_PROPERTY_PERSISTENT_UID,
    MTP_PROPERTY_NAME,
    MTP_PROPERTY_DISPLAY_NAME,
    MTP_PROPERTY_DATE_ADDED,

    // video specific properties
    MTP_PROPERTY_ARTIST,
    MTP_PROPERTY_ALBUM_NAME,
    MTP_PROPERTY_DURATION,
    MTP_PROPERTY_DESCRIPTION
};

int MinMtpDatabase::IMAGE_PROPERTIES[12] = {
    // NOTE must match FILE_PROPERTIES above
    MTP_PROPERTY_STORAGE_ID,
    MTP_PROPERTY_OBJECT_FORMAT,
    MTP_PROPERTY_PROTECTION_STATUS,
    MTP_PROPERTY_OBJECT_SIZE,
    MTP_PROPERTY_OBJECT_FILE_NAME,
    MTP_PROPERTY_DATE_MODIFIED,
    MTP_PROPERTY_PARENT_OBJECT,
    MTP_PROPERTY_PERSISTENT_UID,
    MTP_PROPERTY_NAME,
    MTP_PROPERTY_DISPLAY_NAME,
    MTP_PROPERTY_DATE_ADDED,

    // image specific properties
    MTP_PROPERTY_DESCRIPTION
};

int MinMtpDatabase::ALL_PROPERTIES[25] = {
    // NOTE must match FILE_PROPERTIES above
    MTP_PROPERTY_STORAGE_ID,
    MTP_PROPERTY_OBJECT_FORMAT,
    MTP_PROPERTY_PROTECTION_STATUS,
    MTP_PROPERTY_OBJECT_SIZE,
    MTP_PROPERTY_OBJECT_FILE_NAME,
    MTP_PROPERTY_DATE_MODIFIED,
    MTP_PROPERTY_PARENT_OBJECT,
    MTP_PROPERTY_PERSISTENT_UID,
    MTP_PROPERTY_NAME,
    MTP_PROPERTY_DISPLAY_NAME,
    MTP_PROPERTY_DATE_ADDED,

    // image specific properties
    MTP_PROPERTY_DESCRIPTION,

    // audio specific properties
    MTP_PROPERTY_ARTIST,
    MTP_PROPERTY_ALBUM_NAME,
    MTP_PROPERTY_ALBUM_ARTIST,
    MTP_PROPERTY_TRACK,
    MTP_PROPERTY_ORIGINAL_RELEASE_DATE,
    MTP_PROPERTY_DURATION,
    MTP_PROPERTY_GENRE,
    MTP_PROPERTY_COMPOSER,

    // video specific properties
    MTP_PROPERTY_ARTIST,
    MTP_PROPERTY_ALBUM_NAME,
    MTP_PROPERTY_DURATION,
    MTP_PROPERTY_DESCRIPTION,

    // image specific properties
    MTP_PROPERTY_DESCRIPTION
};

int MinMtpDatabase::SUPPORTED_PLAYBACK_FORMATS[26] = {
    SUPPORTED_PLAYBACK_FORMAT_UNDEFINED,
    SUPPORTED_PLAYBACK_FORMAT_ASSOCIATION,
    SUPPORTED_PLAYBACK_FORMAT_TEXT,
    SUPPORTED_PLAYBACK_FORMAT_HTML,
    SUPPORTED_PLAYBACK_FORMAT_WAV,
    SUPPORTED_PLAYBACK_FORMAT_MP3,
    SUPPORTED_PLAYBACK_FORMAT_MPEG,
    SUPPORTED_PLAYBACK_FORMAT_EXIF_JPEG,
    SUPPORTED_PLAYBACK_FORMAT_TIFF_EP,
    SUPPORTED_PLAYBACK_FORMAT_BMP,
    SUPPORTED_PLAYBACK_FORMAT_GIF,
    SUPPORTED_PLAYBACK_FORMAT_JFIF,
    SUPPORTED_PLAYBACK_FORMAT_PNG,
    SUPPORTED_PLAYBACK_FORMAT_TIFF,
    SUPPORTED_PLAYBACK_FORMAT_WMA,
    SUPPORTED_PLAYBACK_FORMAT_OGG,
    SUPPORTED_PLAYBACK_FORMAT_AAC,
    SUPPORTED_PLAYBACK_FORMAT_MP4_CONTAINER,
    SUPPORTED_PLAYBACK_FORMAT_MP2,
    SUPPORTED_PLAYBACK_FORMAT_3GP_CONTAINER,
    SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_AV_PLAYLIST,
    SUPPORTED_PLAYBACK_FORMAT_WPL_PLAYLIST,
    SUPPORTED_PLAYBACK_FORMAT_M3U_PLAYLIST,
    SUPPORTED_PLAYBACK_FORMAT_PLS_PLAYLIST,
    SUPPORTED_PLAYBACK_FORMAT_XML_DOCUMENT,
    SUPPORTED_PLAYBACK_FORMAT_FLAC
};

MtpObjectHandle MinMtpDatabase::beginSendObject(const char* path,
                                            MtpObjectFormat format,
                                            MtpObjectHandle parent,
                                            MtpStorageID storageID) {
    ++current_handle;
    objectNameMap[current_handle] = path;

    return current_handle;
}

void MinMtpDatabase::endSendObject(MtpObjectHandle handle, bool succeeded) {
    if (!succeeded) {
        LOG(ERROR) << "endSendObject() failed, unlinking ";
    }
}

MtpObjectHandleList* MinMtpDatabase::getObjectList(MtpStorageID storageID,
                                    MtpObjectFormat format,
                                    MtpObjectHandle parent) {
    MtpObjectHandleList* list = new MtpObjectHandleList();
    if (parent == MTP_PARENT_ROOT) {
        // handle id start from 1
        std::map<MtpObjectHandle, std::string>::iterator iter;
        for (iter = objectNameMap.begin(); iter != objectNameMap.end(); iter++) {
            list->push_back(iter->first);
        }
        return list;
    } else {
        //if not root, do nothing.
        return list;
    }
}

int MinMtpDatabase::getNumObjects(MtpStorageID storageID,
                                    MtpObjectFormat format,
                                    MtpObjectHandle parent) {
    int result = current_handle;
    //get number of objects on filesystem storage
    return result;
}

MtpObjectFormatList* MinMtpDatabase::getSupportedPlaybackFormats() {
    // This function tells the host PC which file formats the device supports
    int* formats;
    MtpObjectFormatList* list = new MtpObjectFormatList();
    formats = SUPPORTED_PLAYBACK_FORMATS;
    int length = sizeof(SUPPORTED_PLAYBACK_FORMATS) / sizeof(int);
    for (int i = 0; i < length; i++) {
        list->push_back(formats[i]);
    }
    return list;
}

MtpObjectFormatList* MinMtpDatabase::getSupportedCaptureFormats() {
    // Android OS implementation of this function returns NULL
    // so we are not implementing this function either.
    LOG(ERROR) << "MinMtpDatabase::getSupportedCaptureFormats returning NULL (This is what Android does as well).";
    return NULL;
}

MtpObjectPropertyList* MinMtpDatabase::getSupportedObjectProperties(MtpObjectFormat format) {
    int* properties;
    MtpObjectPropertyList* list = new MtpObjectPropertyList();
    properties = FILE_PROPERTIES;
    int length = sizeof(FILE_PROPERTIES) / sizeof(FILE_PROPERTIES[0]);
    for (int i = 0; i < length; i++) {
        list->push_back(properties[i]);
    }
    return list;
}

MtpDevicePropertyList* MinMtpDatabase::getSupportedDeviceProperties() {
    int properties[] = {
        MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER,
        MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME,
        MTP_DEVICE_PROPERTY_IMAGE_SIZE,
    };
    MtpDevicePropertyList* list = new MtpDevicePropertyList();
    int length = sizeof(properties) / sizeof(int);
    length = 3;
    for (int i = 0; i < length; i++)
        list->push_back(properties[i]);
    return list;
}

int* MinMtpDatabase::getSupportedObjectProperties(int format) {
    switch (format) {
        case MTP_FORMAT_MP3:
        case MTP_FORMAT_WMA:
        case MTP_FORMAT_OGG:
        case MTP_FORMAT_AAC:
            return AUDIO_PROPERTIES;
        case MTP_FORMAT_MPEG:
        case MTP_FORMAT_3GP_CONTAINER:
        case MTP_FORMAT_WMV:
            return VIDEO_PROPERTIES;
        case MTP_FORMAT_EXIF_JPEG:
        case MTP_FORMAT_GIF:
        case MTP_FORMAT_PNG:
        case MTP_FORMAT_BMP:
            return IMAGE_PROPERTIES;
        case 0:
            return ALL_PROPERTIES;
        default:
            return FILE_PROPERTIES;
    }
}

MtpResponseCode MinMtpDatabase::getObjectPropertyValue(MtpObjectHandle handle,
                                            MtpObjectProperty property,
                                            MtpDataPacket& packet) {
    // this function is fake, returned properity is always zero.
    int type;
    MtpResponseCode result = 0;
    uint64_t longValue = 0;//added = 0
    if (!getObjectPropertyInfo(property, type)) {
        LOG(ERROR) << "MinMtpDatabase::getObjectPropertyValue returning MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED";
        return MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED;
    }

    // special case date properties, which are strings to MTP
    // but stored internally as a uint64
    if (property == MTP_PROPERTY_DATE_MODIFIED || property == MTP_PROPERTY_DATE_ADDED) {
        char date[20];
        //formatDateTime(longValue, date, sizeof(date));
        packet.putString(date);
        goto out;
    }
    // release date is stored internally as just the year
    if (property == MTP_PROPERTY_ORIGINAL_RELEASE_DATE) {
        char date[20];
        snprintf(date, sizeof(date), "%04lld0101T000000", (long long int)longValue);
        packet.putString(date);
        goto out;
    }

    switch (type) {
        case MTP_TYPE_INT8:
            packet.putInt8(longValue);
            break;
        case MTP_TYPE_UINT8:
            packet.putUInt8(longValue);
            break;
        case MTP_TYPE_INT16:
            packet.putInt16(longValue);
            break;
        case MTP_TYPE_UINT16:
            packet.putUInt16(longValue);
            break;
        case MTP_TYPE_INT32:
            packet.putInt32(longValue);
            break;
        case MTP_TYPE_UINT32:
            packet.putUInt32(longValue);
            break;
        case MTP_TYPE_INT64:
            packet.putInt64(longValue);
            break;
        case MTP_TYPE_UINT64:
            packet.putUInt64(longValue);
            break;
        case MTP_TYPE_INT128:
            packet.putInt128(longValue);
            break;
        case MTP_TYPE_UINT128:
            packet.putInt128(longValue);
            break;
        case MTP_TYPE_STR:
            {
                LOG(ERROR) << "STRING unsupported type in getObjectPropertyValue";
                break;
            }
        default:
            LOG(ERROR) << "unsupported type in getObjectPropertyValue";
    }
    result = MTP_RESPONSE_INVALID_OBJECT_PROP_FORMAT;
out:
    return result;
}

MtpResponseCode MinMtpDatabase::setObjectPropertyValue(MtpObjectHandle handle,
                                            MtpObjectProperty property,
                                            MtpDataPacket& packet) {
    //this function only could be used to set string type.
    int type;
    if (!getObjectPropertyInfo(property, type)) {
        LOG(ERROR) << "MinMtpDatabase::setObjectPropertyValue returning MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED";
        return MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED;
    }
    long longValue = 0;
    std::string stringValue;

    switch (type) {
        case MTP_TYPE_INT8:
            //do nothing
            break;
        case MTP_TYPE_UINT8:
            //do nothing
            break;
        case MTP_TYPE_INT16:
            //do nothing
            break;
        case MTP_TYPE_UINT16:
            //do nothing
            break;
        case MTP_TYPE_INT32:
            //do nothing
            break;
        case MTP_TYPE_UINT32:
            //do nothing
            break;
        case MTP_TYPE_INT64:
            //do nothing
            break;
        case MTP_TYPE_UINT64:
            //do nothing
            break;
        case MTP_TYPE_STR:
            {
                MtpStringBuffer buffer;
                packet.getString(buffer);
                stringValue = (const char *)buffer;
                break;
             }
        default:
            LOG(ERROR) << "MinMtpDatabase::setObjectPropertyValue unsupported type " << type << "in getObjectPropertyValue";
            return MTP_RESPONSE_INVALID_OBJECT_PROP_FORMAT;
    }
    int result = MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED;

    switch (property) {
        case MTP_PROPERTY_OBJECT_FILE_NAME:
            {
                objectNameMap[handle] = stringValue;
                result = MTP_RESPONSE_OK;
            }
            break;

        default:
            LOG(ERROR) << "MinMtpDatabase::setObjectPropertyValue property " << property << " not supported.";
            result = MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED;
    }
    return result;
}

MtpResponseCode MinMtpDatabase::getDevicePropertyValue(MtpDeviceProperty property,
                                            MtpDataPacket& packet) {
    int type, result = 0;
    char prop_value[PROPERTY_VALUE_MAX];
    if (!getDevicePropertyInfo(property, type)) {
        LOG(ERROR) << "MinMtpDatabase::getDevicePropertyValue MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED";
        return MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED;
    }
    switch (property) {
        case MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER:
        case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME:
            result =  MTP_RESPONSE_OK;
            break;
        default:
        {
            LOG(ERROR) << "MinMtpDatabase::getDevicePropertyValue property " << property << " not supported.";
            result = MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED;
            break;
        }
    }

    if (result != MTP_RESPONSE_OK) {
        LOG(ERROR) << "MTP_REPONSE_OK NOT OK";
        return result;
    }

    long longValue = 0;
    property_get("ro.build.product", prop_value, "unknown manufacturer");
    switch (type) {
        case MTP_TYPE_INT8: {
            packet.putInt8(longValue);
            break;
        }
        case MTP_TYPE_UINT8:
        {
            packet.putUInt8(longValue);
            break;
        }
        case MTP_TYPE_INT16:
        {
            packet.putInt16(longValue);
            break;
        }
        case MTP_TYPE_UINT16:
        {
            packet.putUInt16(longValue);
            break;
        }
        case MTP_TYPE_INT32:
        {
            packet.putInt32(longValue);
            break;
        }
        case MTP_TYPE_UINT32:
        {
            packet.putUInt32(longValue);
            break;
        }
        case MTP_TYPE_INT64:
        {
            packet.putInt64(longValue);
            break;
        }
        case MTP_TYPE_UINT64:
        {
            packet.putUInt64(longValue);
            break;
        }
        case MTP_TYPE_INT128:
        {
            packet.putInt128(longValue);
            break;
        }
        case MTP_TYPE_UINT128:
        {
            packet.putInt128(longValue);
            break;
        }
        case MTP_TYPE_STR:
        {
            char* str = prop_value;
            packet.putString(str);
            break;
         }
        default:
            LOG(ERROR) << "MinMtpDatabase::getDevicePropertyValue unsupported type " << type << " in getDevicePropertyValue";
            return MTP_RESPONSE_INVALID_DEVICE_PROP_FORMAT;
    }

    return MTP_RESPONSE_OK;
}

MtpResponseCode MinMtpDatabase::setDevicePropertyValue(MtpDeviceProperty property, MtpDataPacket& packet) {
       int type;
    LOG(ERROR) << "MinMtpDatabase::setDevicePropertyValue not implemented, returning 0";
    return 0;
}

MtpResponseCode MinMtpDatabase::resetDeviceProperty(MtpDeviceProperty property) {
    LOG(ERROR) << "MinMtpDatabase::resetDeviceProperty not implemented, returning -1";
       return -1;
}

MtpResponseCode MinMtpDatabase::getObjectPropertyList(MtpObjectHandle handle, uint32_t format, uint32_t property, int groupCode, int depth, MtpDataPacket& packet) {

    MtpResponseCode result;
    int count = 0;
    //get all property.
    if (property == MTP_PARENT_ROOT) {
        if (current_handle > 0) {
            struct stat st;
            lstat(objectNameMap[handle].c_str(), &st);

            count = 9;
            packet.putUInt32(count);

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_OBJECT_FILE_NAME);//properity code
            packet.putUInt16(MTP_TYPE_STR);//data type
            packet.putString(objectNameMap[handle].c_str());//properity value

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_OBJECT_SIZE);//properity code
            packet.putUInt16(MTP_TYPE_UINT64);//data type
            packet.putUInt64(st.st_size);//properity value

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_PROTECTION_STATUS);//properity code
            packet.putUInt16(MTP_TYPE_UINT16);//data type
            packet.putUInt16(0);//properity value

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_STORAGE_ID);//properity code
            packet.putUInt16(MTP_TYPE_UINT32);//data type
            packet.putUInt32(1);//properity value

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_OBJECT_FORMAT);//properity code
            packet.putUInt16(MTP_TYPE_UINT16);//data type
            packet.putUInt16(MTP_FORMAT_UNDEFINED);//properity value

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_PERSISTENT_UID);//properity code
            packet.putUInt16(MTP_TYPE_UINT128);//data type
            packet.putUInt128(current_handle + 1);//properity value

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_NAME);//properity code
            packet.putUInt16(MTP_TYPE_STR);//data type
            packet.putString(objectNameMap[handle].c_str());//properity value

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_DATE_ADDED);//properity code
            packet.putUInt16(MTP_TYPE_STR);//data type
            char atimeStr[100];
            sprintf(atimeStr, "%ld" , st.st_atime);
            packet.putString(atimeStr);//properity value

            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_PARENT_OBJECT);//properity code
            packet.putUInt16(MTP_TYPE_UINT32);//data type
            packet.putUInt32(MTP_PARENT_ROOT);//properity value

            return MTP_RESPONSE_OK;
        }
    } else if (property == MTP_PROPERTY_OBJECT_FORMAT) {
        if (objectNameMap.count(handle) != 1){
            LOG(ERROR) << "MinMtpDatabase::getObjectPropertyList MTP_RESPOSNE_INVALID_OBJECT_HANDLE " << handle;
            return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
        } else {
            count = 1;
            packet.putUInt32(count);
            packet.putUInt32(handle);
            packet.putUInt16(MTP_PROPERTY_OBJECT_FORMAT);//properity code
            packet.putUInt16(MTP_TYPE_UINT16);//data type
            packet.putUInt16(MTP_FORMAT_UNDEFINED);//properity value
            return MTP_RESPONSE_OK;
        }
    }
    return MTP_RESPONSE_OK;
}

MtpResponseCode MinMtpDatabase::getObjectInfo(MtpObjectHandle handle, MtpObjectInfo& info) {

    struct stat st;
    uint64_t size;
    info.mStorageID = (*lstorage)[0]->getStorageID();
    if (objectNameMap.count(handle) != 1){
        LOG(ERROR) << "MinMtpDatabase::getObjectInfo MTP_RESPONSE_INVALID_OBJECT_HANDLE " << handle;
        return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
    }

    info.mName = strdup(basename(objectNameMap[handle].c_str()));
    info.mParent = MTP_PARENT_ROOT;//parent is root
    lstat(info.mName, &st);
    size = st.st_size;
    info.mCompressedSize = size;
    info.mDateModified = st.st_mtime;
    if (S_ISDIR(st.st_mode)) {
        info.mFormat = MTP_FORMAT_ASSOCIATION;
    } else {
        info.mFormat = MTP_FORMAT_UNDEFINED;
    }

    return MTP_RESPONSE_OK;
}

void* MinMtpDatabase::getThumbnail(MtpObjectHandle handle, size_t& outThumbSize) {
    MtpStringBuffer path;
    int64_t length;
    MtpObjectFormat format;
    void* result = NULL;
    outThumbSize = 0;
    LOG(ERROR) << "MinMtpDatabase::getThumbnail not implemented, returning 0";
    return 0;
}

MtpResponseCode MinMtpDatabase::getObjectFilePath(MtpObjectHandle handle, MtpStringBuffer& outFilePath, int64_t& outFileLength, MtpObjectFormat& outFormat) {
    struct stat st;
    if (objectNameMap.count(handle) == 1){//file exist
        outFilePath.set(objectNameMap[handle].c_str());
        lstat((const char*)outFilePath, &st);
        outFileLength = st.st_size;
        outFormat = MTP_FORMAT_UNDEFINED;
        return MTP_RESPONSE_OK;
    } else {
        LOG(ERROR) << "MinMtpDatabase::getObjectFilePath file donesn't exist in: " << (const char*)outFilePath;
        return MTP_RESPONSE_OK;
    }
}

void MinMtpDatabase::rescanFile(const char* path, MtpObjectHandle handle, MtpObjectFormat format) {

}
MtpResponseCode MinMtpDatabase::beginCopyObject(MtpObjectHandle handle, MtpObjectHandle newParent,
                                            MtpStorageID newStorage) {
    return MTP_RESPONSE_OK;
}
void MinMtpDatabase::endCopyObject(MtpObjectHandle handle, bool succeeded) {

}

MtpResponseCode MinMtpDatabase::beginDeleteObject(MtpObjectHandle handle) {
       if (objectNameMap.count(handle) == 1){//file exist
        objectNameMap.erase(handle);
        current_handle--;
    } else {
        LOG(ERROR) << "MinMtpDatabase::deleteFile MTP_RESPONSE_INVALID_OBJECT_HANDLE: " << handle;
        return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
    }
    return MTP_RESPONSE_OK;
}

int MinMtpDatabase::openFilePath(const char* path, bool transcode) {
    return 0;
}
void MinMtpDatabase::endDeleteObject(MtpObjectHandle handle, bool succeeded) {
}

MtpResponseCode MinMtpDatabase::beginMoveObject(MtpObjectHandle handle, MtpObjectHandle newParent,
                                            MtpStorageID newStorage) {
    return MTP_RESPONSE_OK;

}

void MinMtpDatabase::endMoveObject(MtpObjectHandle oldParent, MtpObjectHandle newParent,
                                            MtpStorageID oldStorage, MtpStorageID newStorage,
                                            MtpObjectHandle handle, bool succeeded) {

}

struct PropertyTableEntry {
    MtpObjectProperty   property;
    int                 type;
};

static const PropertyTableEntry   kObjectPropertyTable[] = {
    {   MTP_PROPERTY_STORAGE_ID,        MTP_TYPE_UINT32     },
    {   MTP_PROPERTY_OBJECT_FORMAT,     MTP_TYPE_UINT16     },
    {   MTP_PROPERTY_PROTECTION_STATUS, MTP_TYPE_UINT16     },
    {   MTP_PROPERTY_OBJECT_SIZE,       MTP_TYPE_UINT64     },
    {   MTP_PROPERTY_OBJECT_FILE_NAME,  MTP_TYPE_STR        },
    {   MTP_PROPERTY_DATE_MODIFIED,     MTP_TYPE_STR        },
    {   MTP_PROPERTY_PARENT_OBJECT,     MTP_TYPE_UINT32     },
    {   MTP_PROPERTY_PERSISTENT_UID,    MTP_TYPE_UINT128    },
    {   MTP_PROPERTY_NAME,              MTP_TYPE_STR        },
    {   MTP_PROPERTY_DISPLAY_NAME,      MTP_TYPE_STR        },
    {   MTP_PROPERTY_DATE_ADDED,        MTP_TYPE_STR        },
    {   MTP_PROPERTY_ARTIST,            MTP_TYPE_STR        },
    {   MTP_PROPERTY_ALBUM_NAME,        MTP_TYPE_STR        },
    {   MTP_PROPERTY_ALBUM_ARTIST,      MTP_TYPE_STR        },
    {   MTP_PROPERTY_TRACK,             MTP_TYPE_UINT16     },
    {   MTP_PROPERTY_ORIGINAL_RELEASE_DATE, MTP_TYPE_STR    },
    {   MTP_PROPERTY_GENRE,             MTP_TYPE_STR        },
    {   MTP_PROPERTY_COMPOSER,          MTP_TYPE_STR        },
    {   MTP_PROPERTY_DURATION,          MTP_TYPE_UINT32     },
    {   MTP_PROPERTY_DESCRIPTION,       MTP_TYPE_STR        },
};

static const PropertyTableEntry   kDevicePropertyTable[] = {
    {   MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER,    MTP_TYPE_STR },
    {   MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME,       MTP_TYPE_STR },
    {   MTP_DEVICE_PROPERTY_IMAGE_SIZE,                 MTP_TYPE_STR },
};

bool MinMtpDatabase::getObjectPropertyInfo(MtpObjectProperty property, int& type) {
    int count = sizeof(kObjectPropertyTable) / sizeof(kObjectPropertyTable[0]);
    const PropertyTableEntry* entry = kObjectPropertyTable;
    for (int i = 0; i < count; i++, entry++) {
        if (entry->property == property) {
            type = entry->type;
            return true;
        }
    }
    return false;
}

bool MinMtpDatabase::getDevicePropertyInfo(MtpDeviceProperty property, int& type) {
    int count = sizeof(kDevicePropertyTable) / sizeof(kDevicePropertyTable[0]);
    const PropertyTableEntry* entry = kDevicePropertyTable;
    for (int i = 0; i < count; i++, entry++) {
        if (entry->property == property) {
            type = entry->type;
            return true;
        }
    }
    return false;
}

MtpObjectHandleList* MinMtpDatabase::getObjectReferences(MtpObjectHandle handle) {
    // call function and place files with associated handles into int array
    LOG(ERROR) << "MinMtpDatabase::getObjectReferences returning null, this seems to be what Android always does.";
    // Windows + Android seems to always return a NULL in this function, c == null path
    // The way that this is handled in Android then is to do this:
    return NULL;
}

MtpResponseCode MinMtpDatabase::setObjectReferences(MtpObjectHandle handle,
                                                    MtpObjectHandleList* references) {
    int count = references->size();
    LOG(ERROR) << "MinMtpDatabase::setObjectReferences not implemented, returning 0";
    return 0;
}

MtpProperty* MinMtpDatabase::getObjectPropertyDesc(MtpObjectProperty property,
                                            MtpObjectFormat format) {
    MtpProperty* result = NULL;
    switch (property) {
        case MTP_PROPERTY_OBJECT_FORMAT:
            // use format as default value
            result = new MtpProperty(property, MTP_TYPE_UINT16, false, format);
            break;
        case MTP_PROPERTY_PROTECTION_STATUS:
        case MTP_PROPERTY_TRACK:
            result = new MtpProperty(property, MTP_TYPE_UINT16);
            break;
        case MTP_PROPERTY_STORAGE_ID:
        case MTP_PROPERTY_PARENT_OBJECT:
        case MTP_PROPERTY_DURATION:
            result = new MtpProperty(property, MTP_TYPE_UINT32);
            break;
        case MTP_PROPERTY_OBJECT_SIZE:
            result = new MtpProperty(property, MTP_TYPE_UINT64);
            break;
        case MTP_PROPERTY_PERSISTENT_UID:
            result = new MtpProperty(property, MTP_TYPE_UINT128);
            break;
        case MTP_PROPERTY_NAME:
        case MTP_PROPERTY_DISPLAY_NAME:
        case MTP_PROPERTY_ARTIST:
        case MTP_PROPERTY_ALBUM_NAME:
        case MTP_PROPERTY_ALBUM_ARTIST:
        case MTP_PROPERTY_GENRE:
        case MTP_PROPERTY_COMPOSER:
        case MTP_PROPERTY_DESCRIPTION:
            result = new MtpProperty(property, MTP_TYPE_STR);
            break;
        case MTP_PROPERTY_DATE_MODIFIED:
        case MTP_PROPERTY_DATE_ADDED:
        case MTP_PROPERTY_ORIGINAL_RELEASE_DATE:
            result = new MtpProperty(property, MTP_TYPE_STR);
            result->setFormDateTime();
            break;
        case MTP_PROPERTY_OBJECT_FILE_NAME:
            // We allow renaming files and folders
            result = new MtpProperty(property, MTP_TYPE_STR, true);
            break;
    }
    return result;
}

MtpProperty* MinMtpDatabase::getDevicePropertyDesc(MtpDeviceProperty property) {
    MtpProperty* result = NULL;
    int ret;
    bool writable = true;
    switch (property) {
        case MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER:
        case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME:
            ret = MTP_RESPONSE_OK;
            // fall through
        case MTP_DEVICE_PROPERTY_IMAGE_SIZE:
            result = new MtpProperty(property, MTP_TYPE_STR, writable);
            ret = MTP_RESPONSE_OK;

            // get current value
            if (ret == MTP_RESPONSE_OK) {
                result->setCurrentValue((const uint16_t *)'\0');
                result->setDefaultValue((const uint16_t *)'\0');
            } else {
                LOG(ERROR) << "unable to read device property, response: " << ret;
            }
            break;
        default:
            ret = MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED;
            break;
        }

    return result;
}

void MinMtpDatabase::sessionStarted() {
    LOG(ERROR) << "MinMtpDatabase::sessionStarted not implemented or does nothing, returning";
    return;
}

void MinMtpDatabase::sessionEnded() {
    LOG(ERROR) << "MinMtpDatabase::sessionEnded not implemented or does nothing, returning";
    return;
}

// ----------------------------------------------------------------------------
