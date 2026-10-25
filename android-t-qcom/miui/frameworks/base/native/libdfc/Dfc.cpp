#include <cutils/log.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <iostream>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <fstream>
#include <errno.h>
#include <utils/Errors.h>
#include <streambuf>

#include "Dfc.h"

JsonValue DB1, DB2, DB3, DB4;
bool stopDFCScan = false;
bool stopDFCCompress = false;
bool stopDFCGetDup = false;
bool is_trans = false;
bool enable_log = false;
DfcNativeServiceScanStatus DfcScanStatus = DFC_SCAN_SLEEP;
DfcNativeServiceCompressStatus DfcCompressStatus = DFC_COMPRESS_SLEEP;
DfcNativeServiceGetDupFilesStatus DfcGetDupStatus = DFC_GETDUP_SLEEP;
DfcNativeServiceRestoreStatus DfcRestoreStatus = DFC_RESTORE_SLEEP;

vector<JsonValue> dup_file_list;

string ito_16str(u64 n){
	string str;
	stringstream ss;
	ss<<hex<<n;
	ss>>str;
    return str;
}

void get_time(time_t& cur_time, int& tm_day)
{
    struct tm *lt;

    cur_time =time(NULL);
    lt = localtime(&cur_time);
    tm_day = lt->tm_mday;
}

string subFileExtention(const string path){
    string result;

    if(path.rfind(".") != string::npos){
        result = path.substr(path.rfind(".") + 1);
    }
    if(result.find("/") != string::npos || result.size() > 10 || result.empty())
        result = "nosuffix";

    return result;
}
JsonValue readFileDBJson(const string DBJsonPath){
    ifstream fs;
    ofstream fs_creat;
    JsonReader JReader;
    JsonValue root;
    char * DBJsonPath_t;

    DBJsonPath_t = const_cast<char*>(DBJsonPath.c_str());
    if(access(DBJsonPath_t, 0) != 0){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] creat db file %s", DBJsonPath_t);
        fs_creat.open(DBJsonPath_t, ios::app);
        if(!fs_creat.is_open()){
            DFC_NATIVE_ERR_MSG("[DFC-NATIVE] the file %s creat fail.", DBJsonPath_t);
        }
        fs_creat << "{}" << endl; // Initialize the json file
        fs_creat.close();
    }

    fs.open(DBJsonPath_t, ios::in);
    if(!fs.is_open()){
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] the file %s open fail.", DBJsonPath_t);
        return root;
    }

    if(JReader.parse(fs,root)){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the file %s read success", DBJsonPath_t);
    } else {
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] the file %s read fail", DBJsonPath_t);
    }

    fs.close();
    return root;
}

void DBReaderInit(const string SpaceIDPath, bool isReadDB1, bool isReadDB2, bool isReadDB3, bool isReadDB4){
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] DBReaderInit");
    if(isReadDB1){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] isReadDB1.");
        DB1.clear();
        DB1 = readFileDBJson(SpaceIDPath + "/" + DB1_FILE_INFO_NAME);
    }

    if(isReadDB2){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] isReadDB2.");
        DB2.clear();
        DB2 = readFileDBJson(SpaceIDPath + "/" + DB2_CHECK_NOCOMPRESS_NAME);
    }

    if(isReadDB3){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] isReadDB3.");
        DB3.clear();
        DB3 = readFileDBJson(SpaceIDPath + "/" + DB3_CHECK_COMPRESS_NAME);
    }

    if(isReadDB4){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] isReadDB4.");
        DB4.clear();
        DB4 = readFileDBJson(SpaceIDPath + "/" + DB4_MODIFY_FILE_NAME);
    }
}

void DBWriteUpdate(const string SpaceIDPath, bool isWriteDB1, bool isWriteDB2, bool isWriteDB3, bool isWriteDB4, bool rs_flag){
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] DBWrite");
    vector<JsonValue> JsonDB = {DB1, DB2, DB3, DB4};
    vector<bool> isWriteDB = {isWriteDB1, isWriteDB2, isWriteDB3, isWriteDB4};
    vector<string> DBName = {DB1_FILE_INFO_NAME, DB2_CHECK_NOCOMPRESS_NAME, DB3_CHECK_COMPRESS_NAME, DB4_MODIFY_FILE_NAME};

    for(int i = 0; i < JsonDB.size(); i++){
        if(isWriteDB[i]){
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] write DB%d.", i + 1);
            ofstream path;
            JsonFWiter DBJsonFWrite;
            string eachDBPath = SpaceIDPath + "/" + DBName[i];

            path.open(eachDBPath.c_str(), ios::out);
            if(!path.is_open()){
                DFC_NATIVE_ERR_MSG("[DFC-NATIVE] The DB%d open fail.", i + 1);
                continue;
            }
#ifdef STRESS_TEST
            if(rs_flag){
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] restore");
                JsonDB[i].clear();
            }
#endif
            path << DBJsonFWrite.write(JsonDB[i]);

            path.close();
        }
    }
}

bool getStopProp(string PropName){
    string propResult = android::base::GetProperty(PropName.c_str(), "false");

    if(propResult == "true"){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] prop stop");
        return true;
    } else{
        return false;
    }
}

vector<string> getExceptRiles(){
    vector<string> result;
    ifstream fsRead;
    JsonReader JReader;
    JsonValue root;

    if(access(DFC_EXCEPT_RULES_PATH, 0) == 0){
        fsRead.open(DFC_EXCEPT_RULES_PATH, ios::in);
        if(!fsRead.is_open()){
            DFC_NATIVE_ERR_MSG("[DFC-NATIVE] the except file open fail");
            return result;
        }
        if(JReader.parse(fsRead, root)){
            for(int i = 0; i < root.size(); i++){
                result.push_back(root[i].asString());
            }
        } else {
            DFC_NATIVE_ERR_MSG("[DFC-NATIVE] the except rules analysis fail");
            goto out;
        }
    }
out:
    if(fsRead.is_open())
        fsRead.close();
    return result;
}

int calculateFileMD5(const string file_path, const string file_name, struct stat st, vector<string> fileRules_reg, vector<string> exceptRules){

    string path_name_t = file_name;
    string file_extension_t;
    JsonValue fileinfo, array;
    bool calMD5 = false;

    if(file_name.size() == 0){
        path_name_t = file_path;
    }

    file_extension_t = subFileExtention(path_name_t);

    if(DB1.isMember(file_path)){
#ifndef STRESS_TEST
        if(DB1[file_path][DB1_FILEINFO_KEY_FILE_ISCALMD5].asInt() == 1){
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the file hash been cal md5.");
            return 0;
        }
#endif
    }
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] path: %s", const_cast<char*>(file_path.c_str()));

    if(!fileRules_reg.empty()){
        for(int eachReg = 0; eachReg < fileRules_reg.size(); eachReg++){
            if(regex_match(file_path, regex(fileRules_reg[eachReg]))){
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the file is in cal list");
                calMD5 = true;
                break;
            }
        }
        //Record fileinfo only for files that meet the rules
        if(calMD5){
            fileinfo[DB1_FILEINFO_KEY_VERSION] = 1;
            fileinfo[DB1_FILEINFO_KEY_ST_TIME] = st.st_mtim.tv_sec;
            fileinfo[DB1_FILEINFO_KEY_DEDUPLICATION_FLAG] = 0;
            fileinfo[DB1_FILEINFO_KEY_WRITE_FLAG] = 0;
            fileinfo[DB1_FILEINFO_KEY_FILE_EXTENTION] = file_extension_t;
            fileinfo[DB1_FILEINFO_KEY_FILE_ISCALMD5] = 0;
            DB1[file_path] = fileinfo;
        }
        if(calMD5 && !exceptRules.empty()){
            for(int exRul = 0; exRul < exceptRules.size(); exRul++){
                if(regex_match(file_path, regex(exceptRules[exRul]))){
                    DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the file is in black list");
                    calMD5 = false;
                    break;
                }
            }
        }
    }
    if(calMD5) {
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] computeFileMD5");
        struct md5Data file_md5_t = computeFileMD5(file_path.c_str(), enable_log);
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] file: %s , MD5: %s, error code: %d .", const_cast<char*>(file_path.c_str()), file_md5_t.hash, file_md5_t.errCode);
        if(file_md5_t.errCode != 0){
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] cal md5 error code != 0, continue");
            return 0;
        }
        DB1[file_path][DB1_FILEINFO_KEY_FILE_ISCALMD5] = 1;
        if(DB2.isMember(file_md5_t.hash)){
            bool isAppend = true;
            //update file_list
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] file same");
            if(DB2[file_md5_t.hash][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST].size() >= 1){
                for(int file_ = 0; file_ < DB2[file_md5_t.hash][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST].size(); file_++){
                    if(file_path == DB2[file_md5_t.hash][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST][file_].asString()){
                        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] file same with DB, continue");
                        isAppend = false;
                        break;
                    }
                }
            }
            if(isAppend)
            	DB2[file_md5_t.hash][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST].append(file_path);
        } else {
            //add to DB2
            fileinfo.clear();
            array.clear();
            fileinfo[DB2_CHECK_NOCOMPRESS_KEY_VERSION] = 1;
            fileinfo[DB2_CHECK_NOCOMPRESS_KEY_FILE_SIZE] = st.st_size;
            array.append(file_path);
            fileinfo[DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST] = array;
            DB2[file_md5_t.hash] = fileinfo;
        }
    }

    return 0;
}

int scanSpaceID(const char* path, vector<string> fileRules_reg, vector<string> exceptRules){
    DIR *pDir;
    int ret = 0;
    struct dirent *ptr;
    struct stat st;
    if (!(pDir = opendir(path))){
        ret = stat(path, &st);
        if(ret){
            //Confirm whether there is no permission or a single file
            DFC_NATIVE_ERR_MSG("[DFC-NATIVE:stat] The directory cannot be opened.");
            return -1;
        }

        else if ((!S_ISDIR(st.st_mode)) && st.st_size != 0){
            calculateFileMD5((string)path, "", st, fileRules_reg, exceptRules);
            return 0;
        }
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE:opendir] The directory cannot be opened.");
        return -1;
    }
    while ((ptr = readdir(pDir)) != 0){
        if(getStopProp(DFC_STOP_SCAN_PROP)){
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] stop.");
            goto out;
        }
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0){
            string temp = (string)path + "/" + (string)(ptr->d_name);
            stat(temp.c_str(), &st);
            if ((!S_ISDIR(st.st_mode)) && st.st_size != 0){
                string file_name_ = ptr->d_name;
                calculateFileMD5(temp, file_name_, st, fileRules_reg, exceptRules);
            }else{
                scanSpaceID(temp.c_str(), fileRules_reg, exceptRules);
            }
        }
    }
out:
    closedir(pDir);
    return 0;
}

void readDupFiles(const string SpacdID, const string path, vector<string> fileRules_reg, u32& sameMD5GroupCount, long& canCompressSize, int& dupFileCount){
    JsonMembers MD5KeyMem;
    struct stat st;
    bool dup_flag = false;
    JsonFWiter fw;
    string ret_str;

    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] readDupFiles start");
    vector<string> del;
    JsonValue array;

    MD5KeyMem = DB2.getMemberNames();
    for(auto md5 : MD5KeyMem){
        u32 eachMD5FileCount = 0;

        if(md5.empty()){
            continue;
        }
        if(DB2[md5].isMember(DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST) && DB2[md5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST].size() > 1){
            for(int eachPath = 0; eachPath < DB2[md5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST].size(); eachPath++){
                string eachPath_t = DB2[md5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST][eachPath].asString();
                if(access(eachPath_t.c_str(), 0) == 0) {
                    stat(eachPath_t.c_str(), &st);
                    if(st.st_mtim.tv_sec == DB1[eachPath_t][DB1_FILEINFO_KEY_ST_TIME].asInt64()){
                        // path + reg
                        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] DB2 file %s.", const_cast<char*>(eachPath_t.c_str()));
                        for(int eachReg = 0; eachReg < fileRules_reg.size(); eachReg++){
                            if(regex_match(eachPath_t, regex(fileRules_reg[eachReg]))){
                                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the file is in cal list");
                                eachMD5FileCount ++;
                                dup_flag = true;
                            }
                        }
                    } else {
                        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] file %s is modified.", const_cast<char*>(eachPath_t.c_str()));
                        DB4[DB2[md5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST][0].asString()][DB4_MODIFY_FILE_KEY_VERSION] = 1;
                        DB4[DB2[md5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST][0].asString()][DB4_MODIFY_FILE_KEY_RECOVERY_FLAG] = 0;
                        del.push_back(md5);
                        break;
                    }
                    //stop
                    if(getStopProp(DFC_STOP_GETDUP_PROP)){
                        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] stop.");
                        dup_flag = false;
                        return;
                    }
                } else {
                    DFC_NATIVE_DBG_MSG("[DFC-NATIVE] file no exist.");
                }
            }
        }
        if(dup_flag && eachMD5FileCount > 1){
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the dup file size is %lld, file count is %d", st.st_size, eachMD5FileCount);
            sameMD5GroupCount ++;
            dupFileCount += eachMD5FileCount;
            stat(DB2[md5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST][0].asCString(), &st);
            canCompressSize = canCompressSize + st.st_size * (eachMD5FileCount - st.st_nlink);
        }
        dup_flag = false;
    }

    if(del.size() > 0){
        for(int i = 0; i < del.size(); i++){
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the md5 %s id remove from DB2.", const_cast<char*>(del[i].c_str()));
            DB2.removeMember(del[i]);
        }
    }
}

void compressFile(const string SpaceID, long& compressedGroupCount, long& compressedSize, int& curCompressFileCount){
    struct stat st;
    int ret;
    JsonMembers MD5KeyMem;
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] compressFile start");

    MD5KeyMem = DB2.getMemberNames();
    for(auto fileMD5 : MD5KeyMem){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the compress md5 %s.", const_cast<char*>(fileMD5.c_str()));
        if(fileMD5.empty())
            continue;
        bool addMD5 = false;
        int fileListSize = DB2[fileMD5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST].size();
        if(fileListSize > 1){
            int sourceIndex = 0;
            for(sourceIndex = 0; sourceIndex < fileListSize; sourceIndex++){
                const char* source_t = DB2[fileMD5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST][sourceIndex].asCString();
                if(access(source_t, 0) == 0){
                    break;
                }
            }
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] sourceIndex %d.", sourceIndex);
            if(sourceIndex >= (fileListSize -1)){
                DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the group is deleted");
                continue;
            }
            const char* source = DB2[fileMD5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST][sourceIndex].asCString();
            for(int i = sourceIndex + 1; i < fileListSize; i++){
                const char* dst = DB2[fileMD5][DB2_CHECK_NOCOMPRESS_KEY_FILE_LIST][i].asCString();
                if(access(dst, 0) != 0){
                    DFC_NATIVE_DBG_MSG("[DFC-NATIVE] file %s is deleted", dst);
                    continue;
                }

                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] createHardLink source path: %s.", source);
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] createHardLink dst path: %s.", dst);

                ret = createHardLink(source, dst, enable_log);
                if (ret){
                    DFC_NATIVE_ERR_MSG("[DFC-NATIVE] createHardLink fail, error code %d", ret);
                } else {
                    addMD5 = true;
                    curCompressFileCount ++;
                    if(curCompressFileCount % COMPRESS_PROGRESS_TRANS_LIMIT == 0){
                        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] set property for compress.prog");
                        android::base::SetProperty("sys.dfcservice.compress.prog", to_string(curCompressFileCount));
                    }
                    compressedSize += DB2[fileMD5][DB2_CHECK_NOCOMPRESS_KEY_FILE_SIZE].asInt64();
                    DFC_NATIVE_INFO_MSG("[DFC-NATIVE]  createHardLink success, compressdSize %d, file size %d", compressedSize, DB2[fileMD5][DB2_CHECK_NOCOMPRESS_KEY_FILE_SIZE].asInt64());
                }
                if(getStopProp(DFC_STOP_COMPRESS_PROP)){
                    if(compressedGroupCount == 0){
                        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the compressedGroupCount is 0, selfadd.");
                        compressedGroupCount ++;
                    }
                    DFC_NATIVE_DBG_MSG("[DFC-NATIVE] compress stop.");
                    return;
                }
            }
            if (addMD5){
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] compressedGroupCount add.");
                compressedGroupCount ++;
            }
            DB3[fileMD5] = DB2[fileMD5];
            DB2.removeMember(fileMD5);
        }
    }
}

void restoreFile(const string SpaceID, const string filePath, vector<string> fileRules_reg, long& result){
    bool reg_flag = false, dir_flag = false;
    JsonMembers MD5KeyMem;
    int ret;

    MD5KeyMem = DB3.getMemberNames();
    for(auto md5 : MD5KeyMem){
        if(md5.empty()){
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] md5 is empty");
            continue;
        }
        if(DB3[md5].isMember(DB3_CHECK_COMPRESS_KEY_FILE_LIST)){
            for(int eachFile = 0; eachFile < DB3[md5][DB3_CHECK_COMPRESS_KEY_FILE_LIST].size(); eachFile++){
                string file_t  = DB3[md5][DB3_CHECK_COMPRESS_KEY_FILE_LIST][eachFile].asString();

                for(int eachReg = 0; eachReg < fileRules_reg.size(); eachReg++){
                    if(regex_match(file_t, regex(fileRules_reg[eachReg]))){
                        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the file is in cal list");
                        reg_flag = true;
                        break;
                    }
                }
                //dir + regs restore
                if(filePath.empty() || (!filePath.empty() && file_t.find(filePath) != string::npos)){
                    dir_flag = true;
                }
            }

            if(reg_flag && dir_flag){
                result = result + DB3[md5][DB3_CHECK_COMPRESS_KEY_FILE_SIZE].asInt64() * (DB3[md5][DB3_CHECK_COMPRESS_KEY_FILE_LIST].size() - 1);
                DFC_NATIVE_DBG_MSG("[DFC-NATIVE] restore file md5 group %s", const_cast<char*>(md5.c_str()));
                for(int rFile = 0; rFile < DB3[md5][DB3_CHECK_COMPRESS_KEY_FILE_LIST].size(); rFile++){
                    string path_tt = DB3[md5][DB3_CHECK_COMPRESS_KEY_FILE_LIST][rFile].asString();
                    DFC_NATIVE_DBG_MSG("[DFC-NATIVE] restoreLinkFile path: %s.", const_cast<char*>(path_tt.c_str()));
                    ret = restoreLinkFile(path_tt.c_str(), enable_log);
                    if(ret){
                        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] restoreLinkFile fail, error code %d", ret);
                    } else {
                        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] restoreLinkFile success");
                    }
                    //DB4 update
                    DB4[path_tt][DB4_MODIFY_FILE_KEY_VERSION] = 1;
                    DB4[path_tt][DB4_MODIFY_FILE_KEY_RECOVERY_FLAG] = 1;
                }
                DB3.removeMember(md5);
            }
        }
        reg_flag = false;
        dir_flag = false;
    }
}

void isEnableToolLog(){
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] isEnableDebugToolAndOpenLog start");
    string enable_log_str = android::base::GetProperty("sys.dfcservice.enablelog.ctrl", "false");
    if(enable_log_str == "true"){
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] enable debug log for so and tool");
        enable_log = true;
    } else{
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] disable debug log for so and tool");
    }
}

/* bool Dfc_isSupportImp(){
    return true;
}

int Dfc_getDFCVersionImp(){
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] Dfc_getDFCVersionImp");
    return DFC_NATIVE_SERVICE_VERSION;
} */

void Dfc_scanDuplicateFilesImp(const string& scanDuplicateFilesRules){
    md5Data FileMD5;
    JsonValue scanRules;
    JsonReader JReader;
    string SpaceID_DBPath;
    int ret = 0;
    vector<string> exceptRules;

    if(DfcGetDupStatus == DFC_GETDUP_ACTIVE || DfcScanStatus == DFC_SCAN_ACTIVE ||
        DfcCompressStatus == DFC_COMPRESS_ACTIVE || DfcRestoreStatus == DFC_RESTORE_ACTIVE){
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the scan has been started");
        return;
    }
    DfcScanStatus = DFC_SCAN_ACTIVE;

    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] Dfc_scanDuplicateFilesImp");
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] scanDuplicateFilesRules: %s", const_cast<char*>(scanDuplicateFilesRules.c_str()));
    android::base::SetProperty(DFC_STOP_SCAN_PROP, "false");

    if(!JReader.parse(scanDuplicateFilesRules, scanRules)){
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] scan rules analysis fail");
        return;
    }
    isEnableToolLog();

    DFC_NATIVE_INFO_MSG("[DFC_NATIVE] get except rules");
    exceptRules = getExceptRiles();

    for(int i = 0; i < scanRules.size(); i++){
        vector<string> fileRules_reg;

        if(scanRules[i].isMember("sid")){
            SpaceID_DBPath = DB_FILE_ROOT_DIR + scanRules[i]["sid"].asString();
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the space id DB path is %s.", const_cast<char*>(SpaceID_DBPath.c_str()));

            if(access(SpaceID_DBPath.c_str(), 0) != 0){
                if(mkdir(SpaceID_DBPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0){
                    DFC_NATIVE_ERR_MSG("[DFC-NATIVE] scan rules mkdir SpaceID_DBPath fail");
                    continue; //goto next space ID
                }
            }
            DBReaderInit(SpaceID_DBPath, true, true, false, true);
        } else {
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] scanRules Incorrect format");
        }

        if(scanRules[i].isMember("regs") && !scanRules[i]["regs"].empty()){
            for(int eachReg = 0; eachReg < scanRules[i]["regs"].size(); eachReg++){
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the fileRules_reg is %s", scanRules[i]["regs"][eachReg].asCString());
                fileRules_reg.push_back(scanRules[i]["regs"][eachReg].asString());
            }
        } else {
             DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the fileRules_reg is empty");
             continue;
        }
        if(scanRules[i].isMember("dirs") && !scanRules[i]["dirs"].empty()){
            for(int each_path = 0; each_path < scanRules[i]["dirs"].size(); each_path++){ 
                if(getStopProp(DFC_STOP_SCAN_PROP)){
                    DFC_NATIVE_DBG_MSG("[DFC-NATIVE] stop.");
                    stopDFCScan = false;
                    DBWriteUpdate(SpaceID_DBPath, true, true, false, true, false);
                    DfcScanStatus = DFC_SCAN_SLEEP;
                    return;
                }
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the dir %s.", scanRules[i]["dirs"][each_path].asCString());
                ret = scanSpaceID(scanRules[i]["dirs"][each_path].asCString(), fileRules_reg, exceptRules);
                if(ret){
                    DFC_NATIVE_ERR_MSG("[DFC-NATIVE] scanSpaceID fail");
                    continue;
                }
            }
        } else {
            DFC_NATIVE_ERR_MSG("[DFC-NATIVE] scanRules Incorrect format");
        }
        //update DB and clear
        DBWriteUpdate(SpaceID_DBPath, true, true, false, true, false);
    }
    DfcScanStatus = DFC_SCAN_SLEEP;
    return;
}

string Dfc_getDuplicateFilesImp(const string& getDuplicateFilesRules){

    JsonValue getDupFileRules, result;
    JsonReader JReader;
    string SpaceID_DBPath, result_str, SpaceID;
    JsonFWiter fwrite_;

    if(DfcGetDupStatus == DFC_GETDUP_ACTIVE || DfcScanStatus == DFC_SCAN_ACTIVE ||
       DfcCompressStatus == DFC_COMPRESS_ACTIVE || DfcRestoreStatus == DFC_RESTORE_ACTIVE){
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the Dfc_getDuplicateFilesImp has been start, return");
        return "{}";
    }
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] Dfc_getDuplicateFilesImp");
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] getDuplicateFilesRules: %s", const_cast<char*>(getDuplicateFilesRules.c_str()));
    DfcGetDupStatus = DFC_GETDUP_ACTIVE;
    android::base::SetProperty(DFC_STOP_GETDUP_PROP, "false");

    if(is_trans){
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] Dfc_getDuplicateFilesImp is transfer");
        goto trans;
    }

    if(!JReader.parse(getDuplicateFilesRules, getDupFileRules)){
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] getDuplicateFilesRules analysis fail");
        return result.asString();
    }

    for(int i = 0; i < getDupFileRules.size(); i++){
        vector<string> fileRules_reg;
        JsonValue array;
        u32 sameMD5GroupCount = 0;
        long canCompressSize = 0;
        int dupFileCount = 0;

        if(getDupFileRules[i].isMember("sid")){
            SpaceID_DBPath = DB_FILE_ROOT_DIR + getDupFileRules[i]["sid"].asString();
            SpaceID = getDupFileRules[i]["sid"].asString();
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the space id is %s.", getDupFileRules[i]["sid"].asCString());
            if(access(SpaceID_DBPath.c_str(), 0) == 0){
                DBReaderInit(SpaceID_DBPath, true, true, false, true);
            } else {
                DFC_NATIVE_ERR_MSG("[DFC-NATIVE] the space id DB path is not access");
                continue;
            }
        } else {
            DFC_NATIVE_ERR_MSG("[DFC-NATIVE] getDupFileRules Incorrect format");
            continue;
        }

        if(getDupFileRules[i].isMember("regs") && !getDupFileRules[i]["regs"].empty()){
            for(int eachReg = 0; eachReg < getDupFileRules[i]["regs"].size(); eachReg++){
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the getDupFileRules reg is %s", getDupFileRules[i]["regs"][eachReg].asCString());
                fileRules_reg.push_back(getDupFileRules[i]["regs"][eachReg].asString());
            }
        } else {
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the getDupFileRules reg is empty");
            continue;
        }

        if(getDupFileRules[i].isMember("dirs")){
            if(!getDupFileRules[i]["dirs"].empty()){
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the path is not empty");
                for(int each_path = 0; each_path < getDupFileRules[i]["dirs"].size(); each_path++){
                    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the path %s.", getDupFileRules[i]["dirs"][each_path].asCString());
                    readDupFiles(getDupFileRules[i]["sid"].asString(), getDupFileRules[i]["dirs"][each_path].asString(), fileRules_reg, sameMD5GroupCount, canCompressSize, dupFileCount);
                    //stop
                    if(getStopProp(DFC_STOP_GETDUP_PROP)){
                        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] get dup stop.");
                        break;
                    }
                }
            } else {
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the path is empty");
                readDupFiles(getDupFileRules[i]["sid"].asString(), "", fileRules_reg, sameMD5GroupCount, canCompressSize, dupFileCount);
            }
        } else {
            DFC_NATIVE_ERR_MSG("[DFC-NATIVE] getDupFileRules Incorrect format");
        }
        //update DB and clear 待添加
        DBWriteUpdate(SpaceID_DBPath, true, true, false, true, false);
        array["fileCount"] = dupFileCount;
        array["gCount"] = sameMD5GroupCount;
        array["size"] = canCompressSize;
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] getDupFileRules sameMD5GroupCount %d, canCompressSize %lld, dupFileCount %d.", sameMD5GroupCount, canCompressSize, dupFileCount);
        result[SpaceID] = array;
        string ret_str = fwrite_.write(result);
        if(ret_str.size() > DFC_EACH_TRAN_SIZE){
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the size is larger");
            result["isEnd"] = 0;
            dup_file_list.push_back(result);
            result.clear();
        }

        //stop
        if(getStopProp(DFC_STOP_GETDUP_PROP)){
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] get dup stop.");
            stopDFCGetDup = false;
            break;
        }
    }
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] getDupFileRules done");

    result["isEnd"] = 1;
    dup_file_list.push_back(result);
trans:
    if(dup_file_list.size() > 1){
        is_trans = true;
        result_str = fwrite_.write(dup_file_list[0]);
    }
    if(dup_file_list.size() == 1){
        result_str = fwrite_.write(dup_file_list[0]);
        is_trans = false;
    }

    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] result_str %s.", const_cast<char*>(result_str.c_str()));
    dup_file_list.erase(dup_file_list.begin());
    DfcGetDupStatus = DFC_GETDUP_SLEEP;
    return result_str;
}

string Dfc_compressDuplicateFilesImp(const string& compressDuplicateFilesRules){

    JsonValue comDupFileRules;
    JsonReader JReader;
    string SpaceID_DBPath, result;
    long compressedGroupCount = 0, compressedSize = 0;
    int curCompressFileCount = 0;

    if(DfcGetDupStatus == DFC_GETDUP_ACTIVE || DfcScanStatus == DFC_SCAN_ACTIVE ||
       DfcCompressStatus == DFC_COMPRESS_ACTIVE || DfcRestoreStatus == DFC_RESTORE_ACTIVE){
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the Compress has been started,return");
        return "0,0";
    }

    DfcCompressStatus = DFC_COMPRESS_ACTIVE;
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] recovery compress.prog status to 0");
    android::base::SetProperty("sys.dfcservice.compress.prog", "0");
    android::base::SetProperty(DFC_STOP_COMPRESS_PROP, "false");

    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] Dfc_compressDuplicateFilesImp");
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] compressDuplicateFilesRules: %s", const_cast<char*>(compressDuplicateFilesRules.c_str()));

    if(!JReader.parse(compressDuplicateFilesRules, comDupFileRules)){
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] getDuplicateFilesRules analysis fail");
        return "0,0";
    }
    isEnableToolLog();
    for(int i = 0; i < comDupFileRules.size(); i++){

        SpaceID_DBPath = DB_FILE_ROOT_DIR +  comDupFileRules[i].asString();
        DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the space id %s.", comDupFileRules[i].asCString());
        if(access(SpaceID_DBPath.c_str(), 0) == 0){
            DBReaderInit(SpaceID_DBPath, false, true, true, false);
        } else {
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the space id DB path is not access");
            continue;
        }
        // compress
        compressFile(comDupFileRules[i].asString(), compressedGroupCount, compressedSize, curCompressFileCount);

        if(getStopProp(DFC_STOP_COMPRESS_PROP)){
            DFC_NATIVE_DBG_MSG("[DFC-NATIVE] compress stop.");
            stopDFCCompress = false;
            DfcCompressStatus = DFC_COMPRESS_SLEEP;
            DBWriteUpdate(SpaceID_DBPath, false, true, true, false, false);
            break;
        }
        //update DB and clear
        DBWriteUpdate(SpaceID_DBPath, false, true, true, false, false);
    }

    result = to_string(compressedGroupCount) + "," + to_string(compressedSize);
    DFC_NATIVE_DBG_MSG("[DFC-NATIVE] compress result %s.", const_cast<char*>(result.c_str()));

    // recovery compress property status
    DfcCompressStatus = DFC_COMPRESS_SLEEP;

    return result;
}

long Dfc_restoreDuplicateFilesImp(const string& restoreDuplicateFilesRules){
    JsonValue restoreDupFileRules;
    JsonReader JReader;
    string SpaceID_DBPath;
    long result = 0;

    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] Dfc_restoreDuplicateFilesImp");
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] restoreDuplicateFilesRules: %s", const_cast<char*>(restoreDuplicateFilesRules.c_str()));

    if(DfcGetDupStatus == DFC_GETDUP_ACTIVE || DfcScanStatus == DFC_SCAN_ACTIVE ||
       DfcCompressStatus == DFC_COMPRESS_ACTIVE || DfcRestoreStatus == DFC_RESTORE_ACTIVE){
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] the Compress has been started,return");
        return 0;
    }
    if(!JReader.parse(restoreDuplicateFilesRules, restoreDupFileRules)){
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] getDuplicateFilesRules analysis fail");
        return result;
    }
    DfcRestoreStatus = DFC_RESTORE_ACTIVE;
    isEnableToolLog();

    for(int i = 0; i < restoreDupFileRules.size(); i++){
        vector<string> fileRules_reg;

        if(restoreDupFileRules[i].isMember("sid")){
            SpaceID_DBPath = DB_FILE_ROOT_DIR + restoreDupFileRules[i]["sid"].asString();
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the space id is %s.", restoreDupFileRules[i]["sid"].asCString());
            if(access(SpaceID_DBPath.c_str(), 0) == 0){
                DBReaderInit(SpaceID_DBPath, false, false, true, true);
            } else {
                DFC_NATIVE_ERR_MSG("[DFC-NATIVE] the space id DB path is not access");
                continue;
            }
        } else {
            DFC_NATIVE_ERR_MSG("[DFC-NATIVE] comDupFileRules Incorrect format");
            continue;
        }

        if(restoreDupFileRules[i].isMember("regs")){
            for(int eachReg = 0; eachReg < restoreDupFileRules[i]["regs"].size(); eachReg++){
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] the fileRules_reg is %s", restoreDupFileRules[i]["regs"][eachReg].asCString());
                fileRules_reg.push_back(restoreDupFileRules[i]["regs"][eachReg].asString());
            }
        }

        if(restoreDupFileRules[i]["dirs"].empty()){
            //restore
            DFC_NATIVE_INFO_MSG("[DFC-NATIVE] restore and path is empty");
            restoreFile(SpaceID_DBPath, "", fileRules_reg, result);
        } else {
            for(auto eachPath : restoreDupFileRules[i]["dirs"]){
                //restore
                DFC_NATIVE_INFO_MSG("[DFC-NATIVE] restore and path is %s", eachPath.asCString());
                restoreFile(SpaceID_DBPath, eachPath.asString(), fileRules_reg, result);
            }
        }
        //update DB and clear
        DBWriteUpdate(SpaceID_DBPath, false, false, true, true, true);
    }

    DfcRestoreStatus = DFC_RESTORE_SLEEP;
    return result;
}

void Dfc_forceStopScanDuplicateFilesImp(){
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] Dfc_forceStopScanDuplicateFilesImp");
    if(DfcScanStatus == DFC_SCAN_ACTIVE){
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] DFC native service is active, stop");
        stopDFCScan = true;
    }
    return;
}

void Dfc_stopCompressDuplicateFilesImp(){
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] Dfc_stopCompressDuplicateFilesImp");
    if(DfcCompressStatus == DFC_COMPRESS_ACTIVE){
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] DFC native service is active, stop");
        stopDFCCompress = true;
    }
    return;
}

void Dfc_setDFCExceptRulesImp(const string& exceptRules){
    JsonValue DFCExceptRules;
    JsonReader JReader;
    ofstream exceptPath;

    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] Dfc_setDFCExceptRulesImp");
    DFC_NATIVE_INFO_MSG("[DFC-NATIVE] DFCExceptRules: %s", const_cast<char*>(exceptRules.c_str()));

    if(!JReader.parse(exceptRules, DFCExceptRules)){
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] Dfc_setDFCExceptRulesImp analysis fail");
        return;
    }
    if(DFCExceptRules.empty()){
        DFC_NATIVE_DBG_MSG("[DFC-NATIVE] Dfc_setDFCExceptRulesImp is empty");
        return;
    }

    exceptPath.open(DFC_EXCEPT_RULES_PATH, ios::out | ios::trunc);
    if(!exceptPath.is_open()){
        DFC_NATIVE_ERR_MSG("[DFC-NATIVE] the exceptPath open fail");
        return;
    }

    exceptPath << DFCExceptRules.toStyledString();

    exceptPath.close();
    return;
}