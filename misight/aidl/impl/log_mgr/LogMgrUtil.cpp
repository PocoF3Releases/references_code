#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <log_mgr/LogMgrUtil.h>

#include <android-base/logging.h>

namespace aidl::vendor::xiaomi::hardware::misight {
namespace LogMgrUtil {

time_t GetOffsetDay(const int days) {
    return DAY_SECONDS   * days;
}

bool IsValidTime(const time_t& AEndTime, const time_t& ANowTime)
{
    return (AEndTime >= ANowTime);
}

bool CompareTimeStrExpired(const std::string& folderName, 
                                    const time_t& expiredTime,
                                    bool isOldRamdumpFileFormat) {
    std::string finalName;
    if (isOldRamdumpFileFormat) {
        // For old ramdump file format: ramdump_sensor_2022-08-05_18-19-58.tgz
        LOG(DEBUG) << "isOldRamdumpFileFormat, fileName " << folderName;

        char *token = NULL;
        int fileNameLength = folderName.size();
        char fileName[fileNameLength];
        char tmpDate[20];
        strcpy(fileName, folderName.c_str());
        token = strtok(fileName, "_"); // ramdump
        token = strtok(NULL, "_"); // dev_name
        token = strtok(NULL, "_"); // date
        strcpy(tmpDate, token);
        token = strtok(NULL, "."); // time
        if (strstr(token, "_")) { // if time has ext name(prop/kmsg), split
            token = strtok(NULL, "_");
        }

        strcat(tmpDate, "-");
        strcat(tmpDate, token);

        // gotValidDateTime: finalName = 2022-08-05-18-19-58
        finalName = std::string(tmpDate);
    } else {
        finalName = folderName;
    } 
    
    time_t folderTime = TimeUtil::DateStrToTime(finalName);
    LOG(DEBUG) << "folderTime " << folderTime << " expiredTime " << expiredTime;
    return folderTime <= expiredTime;
}

int getZipFileList(std::string path, std::vector<std::string>& files)
{
    DIR *dir;
    struct dirent *ptr;

    if ((dir = opendir(path.c_str())) == NULL)
    {
        LOG(ERROR) << "Open dir error...";
        return 1;
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if (strcmp(ptr->d_name,".") == 0 || strcmp(ptr->d_name,"..") == 0) // ignore . / ..
            continue;
        else if (ptr->d_type == 8 || ptr->d_type == 10) {    // file or lnk_file
            std::string filePath = path + "/" + ptr->d_name;
            files.push_back(filePath);
        } else if (ptr->d_type == 4) { // dir
            std::string filePath = path + "/" + ptr->d_name;
            getZipFileList(filePath, files);
        }
    }
    closedir(dir);
    return 0;
}

std::string generateDfxJsonStr(std::string dirFullPath, long dirSizeBefore, long dirSizeAfter) {
    Json::Value obj;

    obj["DirFullPath"] = dirFullPath;
    obj["DirSizeBefore"] = dirSizeBefore;
    obj["DirSizeAfter"] = dirSizeAfter;

    return JsonUtil::ConvertJsonToStr(obj);
}

}
}
