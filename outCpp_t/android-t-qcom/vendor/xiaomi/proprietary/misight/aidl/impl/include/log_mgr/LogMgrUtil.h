#include <ctime>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <utils/TimeUtil.h>
#include <utils/JsonUtil.h>

namespace aidl::vendor::xiaomi::hardware::misight {
namespace LogMgrUtil {

time_t GetOffsetDay(const int days);
bool IsValidTime(const time_t& AEndTime, const time_t& ANowTime);
bool CompareTimeStrExpired(const std::string& folderName, 
                                    const time_t& expiredTime,
                                    bool isOldRamdumpFileFormat);

int getZipFileList(std::string path, std::vector<std::string>& files);
std::string generateDfxJsonStr(std::string dirFullPath, long dirSizeBefore, long dirSizeAfter);

}
}
