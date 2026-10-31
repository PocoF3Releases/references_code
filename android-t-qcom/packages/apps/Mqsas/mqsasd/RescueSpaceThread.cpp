/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#include "RescueSpaceThread.h"
#include "utils.h"

#include <dirent.h>
#include <iostream>
#include <cstring>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <string>
#include <cutils/log.h>
#include <android/log.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android-base/strings.h>
#include <android-base/stringprintf.h>
#include <android-base/properties.h>
#include <private/android_filesystem_config.h>

using namespace android;
using android::base::GetProperty;
using android::base::GetBoolProperty;
using android::base::unique_fd;

bool RescueSpaceThread::CoreDumpSpaceManage(std::string& dir_path) {
    std::unique_ptr<DIR, decltype(&closedir)> delete_dir(opendir(dir_path.c_str()), closedir);
    if (delete_dir == nullptr) {
        ALOGE("Unable to open directory %s: %s\n", dir_path.c_str(), strerror(errno));
        return false;
    }

    struct dirent* entry = nullptr;
    // 2. Readdir reads out all the files in the directory in sequence
    while ((entry = readdir(delete_dir.get()))) {
        const std::string base_name(entry->d_name);
        const std::string abs_path = dir_path + base_name;
        time_t t = time(NULL);
        if (entry->d_type == DT_REG) {
            struct stat st = {};
            chmod(abs_path.c_str(), (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP));
            stat(abs_path.c_str(), &st);
            if ((time(&t) - st.st_mtime) >= 60*60) {
                if (unlink(abs_path.c_str()) < 0) {
                    ALOGE("Couldn't unlink %s: %s\n", abs_path.c_str(), strerror(errno));
                } else {
                  ALOGW("****** delete file :%s success *****\n", abs_path.c_str());
                }
            }
        }

        else if (entry->d_type == DT_DIR) {
            if ((abs_path == (dir_path +".")) || (abs_path == (dir_path +".."))) {
                continue;
            }
        }
    }
    return true;
}

bool RescueSpaceThread::IsPartitionSpaceRatioFull(std::string& dir_path) {
    double diskrate = 0.0;
    struct statfs sf = {};
    if (statfs(dir_path.c_str(), &sf) == 0) {
        diskrate = ((sf.f_blocks-sf.f_bfree)*100/sf.f_blocks);
    } else {
        ALOGW("failed to get %s diskrate\n",dir_path.c_str());
        return false;
    }

    if(diskrate >= 90.0) {
        ALOGW("Insufficient partition space %lf, please delete the file!!\n", diskrate);
        return true;
    }
    else {
        ALOGW("Rescue Partition space is free, This diskrate is %lf \n", diskrate);
        return false;
    }
}

size_t RescueSpaceThread::GetDirSizeToFile(std::string& dir_path)
{
    size_t size_file = 0;
    std::unique_ptr<DIR, decltype(&closedir)> delete_dir(opendir(dir_path.c_str()), closedir);
    if (delete_dir == nullptr) {
        ALOGE("Unable to open directory %s: %s\n", dir_path.c_str(), strerror(errno));
        return false;
    }

    struct dirent* entry = nullptr;
    // 2. Readdir reads out all the files in the directory in sequence
    while ((entry = readdir(delete_dir.get()))) {
        std::string base_name(entry->d_name);
        std::string abs_path = dir_path + base_name;
        if (entry->d_type == DT_REG) {
            struct stat st = {};
            chmod(abs_path.c_str(), (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP));
            stat(abs_path.c_str(), &st);
            size_file += st.st_size;
        }

        else if (entry->d_type == DT_DIR) {
            if ((abs_path == (dir_path +".")) || (abs_path == (dir_path +".."))) {
                continue;
            }
            std::string dir_abs_path = abs_path + "/";
            size_file += GetDirSizeToFile(dir_abs_path);
        }
    }
    return size_file;
}

bool RescueSpaceThread::GetDirInfoToFile(std::string& dir_path) {
    std::unique_ptr<DIR, decltype(&closedir)> delete_dir(opendir(dir_path.c_str()), closedir);
    if (delete_dir == nullptr) {
        ALOGE("Unable to open directory %s: %s\n", dir_path.c_str(), strerror(errno));
        return false;
    }
    struct dirent* entry = nullptr;
    std::string rescue_infofile_name = dir_path + RESCUE_INFO_FILE;
    while ((entry = readdir(delete_dir.get()))) {
        const std::string base_name(entry->d_name);
        const std::string abs_path = dir_path + base_name;
        const std::string file_info_format = dir_path + base_name + "\t\n";
        if ((abs_path == (dir_path +".")) || (abs_path == (dir_path +".."))) {
                continue;
        }

        int fd = TEMP_FAILURE_RETRY(open(rescue_infofile_name.c_str(), O_CREAT|O_WRONLY|O_NONBLOCK|O_APPEND, 0666));
        if (fd < 0) {
            ALOGW("failed to open %s: %s", rescue_infofile_name.c_str(), strerror(errno));
            return false;
        }

        int ret = TEMP_FAILURE_RETRY(write(fd, file_info_format.c_str(), strlen(file_info_format.c_str())));
        if (ret <= 0) {
            ALOGW("read %s: %s", file_info_format.c_str(), strerror(errno));
        }
        close(fd);
    }
    return true;
}

bool RescueSpaceThread::DeleteOtherFileDIr(std::string& dir_path) {
    // 1. open /mnt/rescue file dir
    ALOGW("*****Trigger the first stage of spatial storage management*****\n");
    std::unique_ptr<DIR, decltype(&closedir)> delete_dir(opendir(dir_path.c_str()), closedir);
    if (delete_dir == nullptr) {
        ALOGE("Unable to open directory %s: %s\n", dir_path.c_str(), strerror(errno));
        return false;
    }

    struct dirent* entry = nullptr;
    // 2. Readdir reads out all the files in the directory in sequence
    while ((entry = readdir(delete_dir.get()))) {
        const std::string base_name(entry->d_name);
        const std::string abs_path = dir_path + base_name;

        if (entry->d_type == DT_REG) {
            if ((base_name.find("boot_flag") == 0)) {
                continue;
            }

            chmod(abs_path.c_str(), (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP));

            if (unlink(abs_path.c_str()) < 0) {
                ALOGE("Couldn't unlink %s: %s\n", abs_path.c_str(), strerror(errno));
            } else {
                ALOGW("****** delete file :%s success *****\n", abs_path.c_str());
            }
        }
        else if (entry->d_type == DT_DIR) {
            if ((abs_path == (dir_path +".")) || (abs_path == (dir_path +".."))) {
                continue;
            }

            if ((abs_path == (dir_path +"mqsas")) || (abs_path == (dir_path +"recovery")) || (abs_path == (dir_path +"update_engine_log")) || (abs_path == (dir_path +"lost+found")) || (abs_path == (dir_path +"system_optimize"))) {
                continue;
            }
            delete_dir_contents_and_dir(abs_path.c_str());
        }
    }
    return true;
}

bool RescueSpaceThread::DeleteSystemOptimFileDIr(std::string& dir_path) {
    // 1. open /mnt/rescue file dir
    ALOGW("*****DeleteSystemOptimFileDIr *****\n");
    std::unique_ptr<DIR, decltype(&closedir)> delete_dir(opendir(dir_path.c_str()), closedir);
    if (delete_dir == nullptr) {
        ALOGW("Unable to open directory %s: %s\n", dir_path.c_str(), strerror(errno));
        return false;
    }
    struct dirent* entry = nullptr;
    // 2. Readdir reads out all the files in the directory in sequence
    while ((entry = readdir(delete_dir.get()))) {
        const std::string base_name(entry->d_name);

        const std::string abs_path = dir_path + base_name;
        if (entry->d_type == DT_REG) {
            chmod(abs_path.c_str(), (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP));
            if (unlink(abs_path.c_str()) < 0) {
                ALOGE("Couldn't unlink %s: %s\n", abs_path.c_str(), strerror(errno));
            } else {
                ALOGW("****** delete file :%s success ******\n", abs_path.c_str());
            }
        } else if (entry->d_type == DT_DIR) {
            if ((abs_path == (dir_path +".")) || (abs_path == (dir_path +".."))) {
                continue;
            }
            delete_dir_contents_and_dir(abs_path.c_str());
        }
    }
    return true;
}

bool RescueSpaceThread::DeleteMqsFileDIr(std::string& dir_path) {
    // 1. open /mnt/rescue file dir
    ALOGW("***** Trigger the second stage of spatial storage management *****\n");
    std::unique_ptr<DIR, decltype(&closedir)> delete_dir(opendir(dir_path.c_str()), closedir);
    if (delete_dir == nullptr) {
        ALOGW("Unable to open directory %s: %s\n", dir_path.c_str(), strerror(errno));
        return false;
    }
    struct dirent* entry = nullptr;
    // 2. Readdir reads out all the files in the directory in sequence
    while ((entry = readdir(delete_dir.get()))) {
        const std::string base_name(entry->d_name);

        const std::string abs_path = dir_path + base_name;
        if (entry->d_type == DT_REG) {

            if ((base_name.find("bm_") == 0) || (base_name.find("BootFailure_") == 0) || (base_name.find("RescueParty_") == 0)) {
                continue;
            }

            chmod(abs_path.c_str(), (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP));
            if (unlink(abs_path.c_str()) < 0) {
                ALOGE("Couldn't unlink %s: %s\n", abs_path.c_str(), strerror(errno));
            } else {
                ALOGW("****** delete file :%s success ******\n", abs_path.c_str());
            }
        } else if (entry->d_type == DT_DIR) {
            if ((abs_path == (dir_path +".")) || (abs_path == (dir_path +"..")) ||(abs_path == (dir_path +"stabilityrecord")) ||(abs_path == (dir_path +"RescuePartyLog"))) {
                continue;
            }

            delete_dir_contents_and_dir(abs_path.c_str());
        }
    }
    return true;
}

bool RescueSpaceThread::DeleteMqsAllFileDIr(std::string& dir_path) {
    // 1. open /mnt/rescue file dir
    ALOGW("***** Trigger the all stage of spatial storage management *****\n");
    std::unique_ptr<DIR, decltype(&closedir)> delete_dir(opendir(dir_path.c_str()), closedir);
    if (delete_dir == nullptr) {
        ALOGW("Unable to open directory %s: %s\n", dir_path.c_str(), strerror(errno));
        return false;
    }
    struct dirent* entry = nullptr;
    // 2. Readdir reads out all the files in the directory in sequence
    while ((entry = readdir(delete_dir.get()))) {
        const std::string base_name(entry->d_name);

        const std::string abs_path = dir_path + base_name;
        if (entry->d_type == DT_REG) {

            if ((base_name.find("rescue_info.log") == 0)) {
                continue;
            }
            chmod(abs_path.c_str(), (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP));
            if (unlink(abs_path.c_str()) < 0) {
                ALOGE("Couldn't unlink %s: %s\n", abs_path.c_str(), strerror(errno));
            } else {
                ALOGW("****** delete file :%s success ******\n", abs_path.c_str());
            }
        } else if (entry->d_type == DT_DIR) {
            if ((abs_path == (dir_path +".")) || (abs_path == (dir_path +"..")) || (abs_path == (dir_path +"stabilityrecord"))) {
                continue;
            }
            delete_dir_contents_and_dir(abs_path.c_str());
        }
    }
    return true;
}

bool RescueSpaceThread::threadLoop() {
    std::string where_space = GetBoolProperty("ro.build.ab_update", false)
                    ? RESCUE_PATH : CACHE_PATH;
    std::string data_miuilog_coredump = "data/miuilog/stability/nativecrash/coredump/";
    for (;;) {
        bool reboot_coredump_prop = GetBoolProperty("persist.reboot.coredump", false);
        bool mtbf_test_prop = GetBoolProperty("persist.mtbf.test", false);
        ALOGW("data/miuilog/stability/nativecrash/coredump space management system starts!!\n");
        ALOGW("data_miuilog_coredump is %d, mtbf_test_prop is %d\n", reboot_coredump_prop, mtbf_test_prop);
        bool skipRemoveCore = mtbf_test_prop || reboot_coredump_prop;
            if (skipRemoveCore) {
                ALOGW("skip delete file(%s) \n", data_miuilog_coredump.c_str());
            } else {
                CoreDumpSpaceManage(data_miuilog_coredump);
                sleep(5);
            }

        ALOGW("Rescue partition space management system starts!!\n");
        if (IsPartitionSpaceRatioFull(where_space)) {
            ALOGW("Insufficient partition space, GradeSpace is first stage\n");
            if (DeleteOtherFileDIr(where_space)) ALOGW("Delete other file dir success!!\n");
            sleep(5);
        }

        if (IsPartitionSpaceRatioFull(where_space)) {
            ALOGW("Insufficient partition space, GradeSpace is second stage\n");
            std::string mqs_dir = where_space + "mqsas/";
            std::string system_optime_dir = where_space + "system_optimize/";
            if (DeleteMqsFileDIr(mqs_dir)) ALOGW("Delete Mqs File dir success!!\n");
            if (GetDirSizeToFile(system_optime_dir) >= 1024*1024*4) {
                ALOGW("********** GetDirSizeToFile = %lu \n", GetDirSizeToFile(system_optime_dir));
                if (DeleteSystemOptimFileDIr(system_optime_dir)) ALOGW("Delete SystemOptimFile File dir success!!\n");

            }
            sleep(5);
        }

        if (IsPartitionSpaceRatioFull(where_space)) {
            ALOGW("Insufficient partition space, GradeSpace is all stage\n");
            std::string mqs_all_dir = where_space + "mqsas/";
            if (GetDirInfoToFile(where_space)) ALOGW("Get file info success!!\n");
            if (DeleteMqsAllFileDIr(mqs_all_dir)) ALOGW("Delete Mqs all success!!\n");
            sleep(5);
        }
        sleep(60*20);
    }
    return false;
}
