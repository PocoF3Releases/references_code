
#ifndef _DFC_H_
#define _DFC_H_
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sstream>
#include <assert.h>
#include <unordered_map>
#include <streambuf>
#include <regex>
#include <android-base/properties.h>

#include "libjsoncpp/json/json.h"

using namespace std;

#define DFC_NATIVE_SERVICE_VERSION 1

#define F2FSDFC_DEBUG 0

//#define STRESS_TEST

#if F2FSDFC_DEBUG
#define DFC_NATIVE_INFO_MSG(msg, args...) do { \
        ALOGI("%s:%d: " msg, __func__, __LINE__, ##args); \
    } while(0)
#else
#define DFC_NATIVE_INFO_MSG(msg, args...) do { } while(0)
#endif

#define DFC_NATIVE_DBG_MSG(msg, args...) ALOGD("%s:%d: " msg, __func__, __LINE__, ##args)

#define DFC_NATIVE_ERR_MSG(msg, args...) ALOGE("%s:%d: " msg, __func__, __LINE__, ##args)

#define MD5SUM_STRING_LEN 32
#define COMPRESS_PROGRESS_TRANS_LIMIT 100

typedef  unsigned int u32;
typedef  unsigned long long u64;
typedef  unordered_map<string, string> MD5HASHMAP;

typedef Json::Writer JsonWriter;
typedef Json::FastWriter JsonFWiter;
typedef Json::StyledWriter JsonSWiter;
typedef Json::Reader JsonReader;
typedef Json::Value  JsonValue;
typedef JsonValue::Members JsonMembers;

struct md5Data {
	int errCode;
	char hash[MD5SUM_STRING_LEN + 1];
};

/*DB1---file info format
  key---file_path
    {
        "file_path1":{
            "V":"1",
            "MD5":"123456789abcdef12345678",
            "s_time":"1978502",
            "w_flag":"1",
            "c_flag":"0"
        },
        "file_path2":{
            "V":"1",
            "MD5":"123456789abcdef12345678",
            "s_time":"1978502",
            "w_flag":"1",
            "c_flag":"0"
        }
    }
*/

#define DB1_FILEINFO_KEY_VERSION "V"
#define DB1_FILEINFO_KEY_ST_TIME "s_time"
#define DB1_FILEINFO_KEY_WRITE_FLAG "w_flag"
#define DB1_FILEINFO_KEY_DEDUPLICATION_FLAG "c_flag"
#define DB1_FILEINFO_KEY_FILE_EXTENTION "reg"
#define DB1_FILEINFO_KEY_FILE_ISCALMD5 "is_md5"

/*DB2---Checked but not de duplicated
  key---MD5
  {
    "MD51":{
        "V":"1",
        "f_size": "1024",
        "f_list":[
            "path1",
            "path2"
        ]
    },
    "MD52":{
        "V":"1",
        "f_size": "1024",
        "f_list":[
            "path1",
            "path2"
        ]
    }
  }
*/

#define DB2_CHECK_NOCOMPRESS_KEY_VERSION "V"
#define DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST "f_list"
#define DB2_CHECK_NOCOMPRESS_KEY_FILE_SIZE "f_size"

/*DB3---Checked but not de duplicated
  key---MD5
  {
    "MD51":{
        "V":"1",
        "f_list":[
            "path1",
            "path2"
        ],
        "f_size":"1024"
    },
    "MD52":{
        "V":"1",
        "f_list":[
            "path1",
            "path2"
        ],
        "f_size":"2048"
    }
  }
*/

#define DB3_CHECK_COMPRESS_KEY_VERSION "V"
#define DB3_CHECK_COMPRESS_KEY_FILE_LIST "f_list"
#define DB3_CHECK_COMPRESS_KEY_FILE_SIZE "f_size"

/*DB4---Checked but not de duplicated
  key---file_path
  {
    "file_path1":{
        "V":"1",
        "r_flag":"0"
    },
    "file_path2":{
        "V":"1",
        "r_flag":"0"
    }
  }
*/

#define DB4_MODIFY_FILE_KEY_VERSION "V"
#define DB4_MODIFY_FILE_KEY_RECOVERY_FLAG "r_flag"

/* DFC DB Path */
#define DB_FILE_ROOT_DIR "/data/local/fbo/"

#define DB1_FILE_INFO_NAME "DFCFileInfo.json"
#define DB2_CHECK_NOCOMPRESS_NAME "DFCCheckNoCompress.json"
#define DB3_CHECK_COMPRESS_NAME "DFCCheckACompress.json"
#define DB4_MODIFY_FILE_NAME "DFCModifyFileInfo.json"

#define DFC_EXCEPT_RULES_PATH "/data/local/fbo/DFCExceptRules.json"

#define DFC_EACH_TRAN_SIZE 600*1024

#define DFC_STOP_SCAN_PROP "sys.dfcservice.stop_scan"
#define DFC_STOP_COMPRESS_PROP "sys.dfcservice.stop_compress"
#define DFC_STOP_GETDUP_PROP "sys.dfcservice.stop_get_duplicate_files"

enum DfcNativeServiceScanStatus{
    DFC_SCAN_ACTIVE = 0,
    DFC_SCAN_SLEEP,
};

enum DfcNativeServiceGetDupFilesStatus{
    DFC_GETDUP_ACTIVE = 0,
    DFC_GETDUP_SLEEP,
};

enum DfcNativeServiceCompressStatus{
    DFC_COMPRESS_ACTIVE = 0,
    DFC_COMPRESS_SLEEP,
};

enum DfcNativeServiceRestoreStatus{
    DFC_RESTORE_ACTIVE = 0,
    DFC_RESTORE_SLEEP,
};

/*
bool Dfc_isSupportImp();
int Dfc_getDFCVersionImp();
*/
void Dfc_scanDuplicateFilesImp(const string& scanDuplicateFilesRules);
string Dfc_getDuplicateFilesImp(const string& getDuplicateFilesRules);
string Dfc_compressDuplicateFilesImp(const string& compressDuplicateFilesRules);
long Dfc_restoreDuplicateFilesImp(const string& restoreDuplicateFilesRules);
void Dfc_forceStopScanDuplicateFilesImp();
void Dfc_stopCompressDuplicateFilesImp();
void Dfc_setDFCExceptRulesImp(const string& exceptRules);

extern "C"
{
    md5Data computeFileMD5(const char *file_name, bool log);
    int createHardLink(const char *sourcePath, const char *destPath, bool log);
    int restoreLinkFile(const char* destPath, bool log);
}

#endif
