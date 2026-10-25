/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#include <utils/threads.h>

namespace android {

// inspect parition path space
#define CACHE_PATH        "/cache/"
#define RESCUE_PATH       "/mnt/rescue/"
#define RESCUE_INFO_FILE  "mqsas/rescue_info.log"

    class RescueSpaceThread : public Thread {
    private:
        bool DeleteOtherFileDIr(std::string& dir_path);
        bool DeleteMqsFileDIr(std::string& dir_path);
        bool DeleteSystemOptimFileDIr(std::string& dir_path);
        bool DeleteMqsAllFileDIr(std::string& dir_path);
        bool GetDirInfoToFile(std::string& dir_path);
        bool IsPartitionSpaceRatioFull(std::string& dir_path);
        bool CoreDumpSpaceManage(std::string& dir_path);
        size_t GetDirSizeToFile(std::string& dir_path);
    protected:
        virtual bool threadLoop();
};

} // namespace android
