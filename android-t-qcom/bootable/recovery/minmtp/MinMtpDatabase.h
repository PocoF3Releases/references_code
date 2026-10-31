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
 *
 * Copyright (C) 2014 TeamWin - bigbiff and Dees_Troy mtp database conversion to C++
 */

#ifndef MINMTPDATABASE_H
#define MINMTPDATABASE_H

#include <utils/Log.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <map>
#include <string>
#include <deque>
#include <android-base/logging.h>

#include "IMtpDatabase.h"
#include "MtpDataPacket.h"
#include "MtpObjectInfo.h"
#include "MtpProperty.h"
#include "MtpStringBuffer.h"
#include "MtpUtils.h"
#include "mtp.h"

// Supported Playback Formats
#define SUPPORTED_PLAYBACK_FORMAT_UNDEFINED 0x3000
/** Format code for associations (folders and directories) */
#define SUPPORTED_PLAYBACK_FORMAT_ASSOCIATION 0x3001
/** Format code for script files */
#define SUPPORTED_PLAYBACK_FORMAT_SCRIPT 0x3002
/** Format code for executable files */
#define SUPPORTED_PLAYBACK_FORMAT_EXECUTABLE 0x3003
/** Format code for text files */
#define SUPPORTED_PLAYBACK_FORMAT_TEXT 0x3004
/** Format code for HTML files */
#define SUPPORTED_PLAYBACK_FORMAT_HTML 0x3005
/** Format code for DPOF files */
#define SUPPORTED_PLAYBACK_FORMAT_DPOF 0x3006
/** Format code for AIFF audio files */
#define SUPPORTED_PLAYBACK_FORMAT_AIFF 0x3007
/** Format code for WAV audio files */
#define SUPPORTED_PLAYBACK_FORMAT_WAV 0x3008
/** Format code for MP3 audio files */
#define SUPPORTED_PLAYBACK_FORMAT_MP3 0x3009
/** Format code for AVI video files */
#define SUPPORTED_PLAYBACK_FORMAT_AVI 0x300A
/** Format code for MPEG video files */
#define SUPPORTED_PLAYBACK_FORMAT_MPEG 0x300B
/** Format code for ASF files */
#define SUPPORTED_PLAYBACK_FORMAT_ASF 0x300C
/** Format code for JPEG image files */
#define SUPPORTED_PLAYBACK_FORMAT_EXIF_JPEG 0x3801
/** Format code for TIFF EP image files */
#define SUPPORTED_PLAYBACK_FORMAT_TIFF_EP 0x3802
/** Format code for BMP image files */
#define SUPPORTED_PLAYBACK_FORMAT_BMP 0x3804
/** Format code for GIF image files */
#define SUPPORTED_PLAYBACK_FORMAT_GIF 0x3807
/** Format code for JFIF image files */
#define SUPPORTED_PLAYBACK_FORMAT_JFIF 0x3808
/** Format code for PICT image files */
#define SUPPORTED_PLAYBACK_FORMAT_PICT 0x380A
/** Format code for PNG image files */
#define SUPPORTED_PLAYBACK_FORMAT_PNG 0x380B
/** Format code for TIFF image files */
#define SUPPORTED_PLAYBACK_FORMAT_TIFF 0x380D
/** Format code for JP2 files */
#define SUPPORTED_PLAYBACK_FORMAT_JP2 0x380F
/** Format code for JPX files */
#define SUPPORTED_PLAYBACK_FORMAT_JPX 0x3810
/** Format code for firmware files */
#define SUPPORTED_PLAYBACK_FORMAT_UNDEFINED_FIRMWARE 0xB802
/** Format code for Windows image files */
#define SUPPORTED_PLAYBACK_FORMAT_WINDOWS_IMAGE_FORMAT 0xB881
/** Format code for undefined audio files files */
#define SUPPORTED_PLAYBACK_FORMAT_UNDEFINED_AUDIO 0xB900
/** Format code for WMA audio files */
#define SUPPORTED_PLAYBACK_FORMAT_WMA 0xB901
/** Format code for OGG audio files */
#define SUPPORTED_PLAYBACK_FORMAT_OGG 0xB902
/** Format code for AAC audio files */
#define SUPPORTED_PLAYBACK_FORMAT_AAC 0xB903
/** Format code for Audible audio files */
#define SUPPORTED_PLAYBACK_FORMAT_AUDIBLE 0xB904
/** Format code for FLAC audio files */
#define SUPPORTED_PLAYBACK_FORMAT_FLAC 0xB906
/** Format code for undefined video files */
#define SUPPORTED_PLAYBACK_FORMAT_UNDEFINED_VIDEO 0xB980
/** Format code for WMV video files */
#define SUPPORTED_PLAYBACK_FORMAT_WMV 0xB981
/** Format code for MP4 files */
#define SUPPORTED_PLAYBACK_FORMAT_MP4_CONTAINER 0xB982
/** Format code for MP2 files */
#define SUPPORTED_PLAYBACK_FORMAT_MP2 0xB983
/** Format code for 3GP files */
#define SUPPORTED_PLAYBACK_FORMAT_3GP_CONTAINER 0xB984
/** Format code for undefined collections */
#define SUPPORTED_PLAYBACK_FORMAT_UNDEFINED_COLLECTION 0xBA00
/** Format code for multimedia albums */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_MULTIMEDIA_ALBUM 0xBA01
/** Format code for image albums */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_IMAGE_ALBUM 0xBA02
/** Format code for audio albums */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_AUDIO_ALBUM 0xBA03
/** Format code for video albums */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_VIDEO_ALBUM 0xBA04
/** Format code for abstract AV playlists */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_AV_PLAYLIST 0xBA05
/** Format code for abstract audio playlists */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_AUDIO_PLAYLIST 0xBA09
/** Format code for abstract video playlists */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_VIDEO_PLAYLIST 0xBA0A
/** Format code for abstract mediacasts */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_MEDIACAST 0xBA0B
/** Format code for WPL playlist files */
#define SUPPORTED_PLAYBACK_FORMAT_WPL_PLAYLIST 0xBA10
/** Format code for M3u playlist files */
#define SUPPORTED_PLAYBACK_FORMAT_M3U_PLAYLIST 0xBA11
/** Format code for MPL playlist files */
#define SUPPORTED_PLAYBACK_FORMAT_MPL_PLAYLIST 0xBA12
/** Format code for ASX playlist files */
#define SUPPORTED_PLAYBACK_FORMAT_ASX_PLAYLIST 0xBA13
/** Format code for PLS playlist files */
#define SUPPORTED_PLAYBACK_FORMAT_PLS_PLAYLIST 0xBA14
/** Format code for undefined document files */
#define SUPPORTED_PLAYBACK_FORMAT_UNDEFINED_DOCUMENT 0xBA80
/** Format code for abstract documents */
#define SUPPORTED_PLAYBACK_FORMAT_ABSTRACT_DOCUMENT 0xBA81
/** Format code for XML documents */
#define SUPPORTED_PLAYBACK_FORMAT_XML_DOCUMENT 0xBA82
/** Format code for MS Word documents */
#define SUPPORTED_PLAYBACK_FORMAT_MS_WORD_DOCUMENT 0xBA83
/** Format code for MS Excel spreadsheets */
#define SUPPORTED_PLAYBACK_FORMAT_MS_EXCEL_SPREADSHEET 0xBA85
/** Format code for MS PowerPoint presentatiosn */
#define SUPPORTED_PLAYBACK_FORMAT_MS_POWERPOINT_PRESENTATION 0xBA86

typedef struct ObjectProperty{
    std::string      filename;
    int              format;
    int              storageID;
    int              parentobject;
} ObjectProperty;

using namespace android;
class MinMtpDatabase : public IMtpDatabase {
private:
    int* getSupportedObjectProperties(int format);

    static int FILE_PROPERTIES[9];
    static int AUDIO_PROPERTIES[19];
    static int VIDEO_PROPERTIES[15];
    static int IMAGE_PROPERTIES[12];
    static int ALL_PROPERTIES[25];
    static int SUPPORTED_PLAYBACK_FORMATS[26];
    std::string lastfile;

    //the handle id of current object, is also the total number of objects.
    int current_handle;
    std::map<MtpObjectHandle, std::string>      objectNameMap;

public:
    MtpStorageList*                 lstorage;
public:
                                    MinMtpDatabase(MtpStorageList * list);
    virtual                         ~MinMtpDatabase();

    //void                            cleanup(JNIEnv *env);
    virtual MtpObjectHandle         beginSendObject(const char* path,
                                            MtpObjectFormat format,
                                            MtpObjectHandle parent,
                                            MtpStorageID storage);

    virtual void                    endSendObject(MtpObjectHandle handle,
                                            bool succeeded);
    
    // Called to rescan a file, such as after an edit.
    virtual void                    rescanFile(const char* path,
                                            MtpObjectHandle handle,
                                            MtpObjectFormat format);

    virtual MtpObjectHandleList*    getObjectList(MtpStorageID storageID,
                                    MtpObjectFormat format,
                                    MtpObjectHandle parent);

    virtual int                     getNumObjects(MtpStorageID storageID,
                                            MtpObjectFormat format,
                                            MtpObjectHandle parent);

    // callee should delete[] the results from these
    // results can be NULL
    virtual MtpObjectFormatList*    getSupportedPlaybackFormats();
    virtual MtpObjectFormatList*    getSupportedCaptureFormats();
    virtual MtpObjectPropertyList*  getSupportedObjectProperties(MtpObjectFormat format);
    virtual MtpDevicePropertyList*  getSupportedDeviceProperties();

    virtual MtpResponseCode         getObjectPropertyValue(MtpObjectHandle handle,
                                            MtpObjectProperty property,
                                            MtpDataPacket& packet);

    virtual MtpResponseCode         setObjectPropertyValue(MtpObjectHandle handle,
                                            MtpObjectProperty property,
                                            MtpDataPacket& packet);

    virtual MtpResponseCode         getDevicePropertyValue(MtpDeviceProperty property,
                                            MtpDataPacket& packet);

    virtual MtpResponseCode         setDevicePropertyValue(MtpDeviceProperty property,
                                            MtpDataPacket& packet);

    virtual MtpResponseCode         resetDeviceProperty(MtpDeviceProperty property);

    virtual MtpResponseCode         getObjectPropertyList(MtpObjectHandle handle,
                                            uint32_t format, uint32_t property,
                                            int groupCode, int depth,
                                            MtpDataPacket& packet);

    virtual MtpResponseCode         getObjectInfo(MtpObjectHandle handle,
                                            MtpObjectInfo& info);

    virtual void*                   getThumbnail(MtpObjectHandle handle, size_t& outThumbSize);

    virtual MtpResponseCode         getObjectFilePath(MtpObjectHandle handle,
                                            MtpStringBuffer& outFilePath,
                                            int64_t& outFileLength,
                                            MtpObjectFormat& outFormat);

    virtual int                     openFilePath(const char* path, bool transcode);
    virtual MtpResponseCode         beginDeleteObject(MtpObjectHandle handle);
    virtual void                    endDeleteObject(MtpObjectHandle handle, bool succeeded);

    bool                            getObjectPropertyInfo(MtpObjectProperty property, int& type);
    bool                            getDevicePropertyInfo(MtpDeviceProperty property, int& type);

    virtual MtpObjectHandleList*    getObjectReferences(MtpObjectHandle handle);

    virtual MtpResponseCode         setObjectReferences(MtpObjectHandle handle,
                                            MtpObjectHandleList* references);

    virtual MtpProperty*            getObjectPropertyDesc(MtpObjectProperty property,
                                            MtpObjectFormat format);

    virtual MtpProperty*            getDevicePropertyDesc(MtpDeviceProperty property);

    virtual MtpResponseCode         beginMoveObject(MtpObjectHandle handle, MtpObjectHandle newParent,
                                            MtpStorageID newStorage);

    virtual void                    endMoveObject(MtpObjectHandle oldParent, MtpObjectHandle newParent,
                                            MtpStorageID oldStorage, MtpStorageID newStorage,
                                            MtpObjectHandle handle, bool succeeded);

    virtual MtpResponseCode         beginCopyObject(MtpObjectHandle handle, MtpObjectHandle newParent,
                                            MtpStorageID newStorage);
    virtual void                    endCopyObject(MtpObjectHandle handle, bool succeeded);

    virtual void                    sessionStarted();

    virtual void                    sessionEnded();

    const char *                    get_file_path();
};
#endif //MINMTPDATABASE_H
