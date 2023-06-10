
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android-base/stringprintf.h>
#include <logwrap/logwrap.h>
#include <selinux/android.h>

#include "utils.h"
#include "miui_datamanager.h"

using android::base::StringPrintf;

namespace android {
namespace installd {

static constexpr const char* kCpPath = "/system/bin/cp";

static bool _set_se_info_r(const char* path, int32_t uid, const char* se_info) {

    if (path == nullptr || se_info == nullptr) {
        LOG(ERROR) << "Skip set seinfo for nullptr.";
        return false;
    }

    int size = strlen(se_info);
    if (size <= 0) {
        LOG(ERROR) << "Skip set seinfo for size = " << size;
        return false;
    }

    if (selinux_android_restorecon_pkgdir(path, se_info, uid, SELINUX_ANDROID_RESTORECON_RECURSE) < 0) {
        LOG(ERROR) << "Could not set seinfo for " << path;
        return false;
    }
    return true;
}

// Helper for chown
static void _chown(const char* path, int32_t uid, int32_t gid) {
    int ret = chown(path, uid, gid);

    if (ret != 0) {
        LOG(ERROR) << "Fail to chown " << path << ", errno=" << strerror(errno);
    }
}

int chown_r(const char* path, int32_t uid, int32_t gid) {
    if (uid == 0 || gid == 0 || path == nullptr) {
        LOG(DEBUG) << "Skip chown due to null path or uid= " << uid << ", gid=" << gid;
        return 0;
    }

    DIR *dir = opendir(path);
    if (dir == nullptr) {
        LOG(ERROR) << "Failed to opendir " << path;
        return -1;
    }

    //1. change the dir first
    _chown(path, uid, gid);

    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr)  {
        if ((strcmp(".", ent->d_name) == 0) || (strcmp("..", ent->d_name) == 0)){
            //skip for "." & ".."
            continue;
        }

        std::string abs_path = StringPrintf("%s/%s", path, ent->d_name);

        if (ent->d_type == DT_DIR) {
            chown_r(abs_path.c_str(), uid, gid);
        } else {
            _chown(abs_path.c_str(), uid, gid);
        }
    }
    closedir(dir);
    return 0;
}

/* Not used: copy from src_dir to dst_base + relative path of src_base (read and write)
int copy_dir_recurisive(const char* src_base, const char* dst_base, const char* src_dir,
        int32_t uid, int32_t gid) {
    //1. copy files first
    int size = strlen(src_base);
    std::string dst_dir = StringPrintf("%s%s", dst_base, src_dir + size);
    int ret = copy_dir_files(src_dir, dst_dir.c_str(), uid, gid);
    if (ret != 0) {
        LOG(ERROR) << "Fail copy : " << src_dir;
        return ret;
    }

    //2. copy other child dir
    DIR *dir = opendir(src_dir);
    if (dir == nullptr) {
        LOG(ERROR) << "Failed to opendir " << src_dir;
        return -1;
    }
    struct dirent *ent;
    while((ent = readdir(dir)) != nullptr)  {
        if ((strcmp(".", ent->d_name) == 0) || (strcmp("..", ent->d_name) == 0)){
            //skip for "." & ".."
            continue;
        }

        if (ent->d_type == DT_DIR) {
            std::string abs_path = StringPrintf("%s/%s", src_dir, ent->d_name);
            copy_dir_recurisive(src_base, dst_base, abs_path.c_str(), uid, gid);
        }
    }
    closedir(dir);
    return 0;
}*/

// see : InstalldNativeService#copy_directory_recursive()
static int32_t copy_directory_recursive(const char* from, const char* to) {
    char *argv[] = {
        (char*) kCpPath,
        (char*) "-F",
        (char*) "-p",
        (char*) "-R",
        (char*) "-P",
        (char*) "-d",
        (char*) from,
        (char*) to
    };

    LOG(DEBUG) << "Copying to " << to;
  //return android_fork_execvp(ARRAY_SIZE(argv), argv, nullptr, false, true);
    return logwrap_fork_execvp(ARRAY_SIZE(argv), argv, nullptr, false, LOG_ALOG, false, nullptr);
}

//delete all the files under the dir, return 0(success) or otherwise (fail)
static int unlink_dir(const char* path, int also_delete_dir/*0 - false, 1- true*/) {
    int ret = delete_dir_contents(path, also_delete_dir, nullptr, true);
    return ret;
}

// Helper for move
static bool unlink_and_rename(const char* from, const char* to) {
    //1. unlink 'to'
    if (unlink_dir(to, 0/*dont delete the root dir*/) == 0) {   // treat it as dir first
        LOG(DEBUG) << "Unlink dir success(exclude root dir): " << to;
    } else {
        if (unlink(to) != 0) {  // unlink as a regular file ?
            LOG(ERROR) << "Unlink failed " << to << " before rename, maybe not exist or is a dir, errno=" << strerror(errno);
        } else {
            LOG(DEBUG) << "Unlink exist before rename:" << to;
        }
    }

    //2. try to rename
    if (rename(from, to) != 0) {
        LOG(ERROR) << "Could not rename to " << to << ", errno=" << strerror(errno);
        return false;
    }
    return true;
}

static bool rename_and_chown(const char* from, const char* to, int32_t uid, int32_t gid, const char* se_info) {
    //1. unlink & rename
    bool success = unlink_and_rename(from, to);

    //2. chown
    if (success) {
        int ret = chown_r(to, uid, gid);
        success &= (ret == 0);
    }

    //3. set se info
    if (success) {
        _set_se_info_r(to, uid, se_info); // current we dont care the ret, will go on even fail
    }

    return success;
}

static bool copy_and_chown(const char* from, const char* to, int32_t uid, int32_t gid, const char* se_info) {

    //1. unlink dir
    if (unlink_dir(to, 1/* delete the root dir*/) == 0) {
        LOG(DEBUG) << "Unlink dir success(include root dir): " << to;
    } else {
        LOG(DEBUG) << "Unlink failed " << to;
        return false;
    }

    //2. do copy
    int ret = copy_directory_recursive(from, to); //copy_dir_recurisive(from, to, from, uid, gid);
    LOG(DEBUG) << "Copy DONE.... " ;
    if (ret != 0) {
        LOG(ERROR) << "Copying may fail " << to << ", errno=" << strerror(errno);
    }

    //3. do chown
    ret = chown_r(to, uid, gid);

    //4. set set info
    if (ret == 0) {
        _set_se_info_r(to, uid, se_info); // current we dont care the ret, will go on even fail
    }
    return (ret == 0);
}

// Move/rename a (from) to a (to) [regular file or dir].
bool move_files(const char* from, const char* to,
        int32_t uid, int32_t gid, const char* se_info, int32_t flag) {

    if (from == nullptr || to == nullptr) {
        LOG(ERROR) << "move_files()...Faild due to from or to is null";
        return false;
    }

    //1. Check whether 'from' exists.
    struct stat s;
    if (stat(from, &s) != 0) {
        LOG(ERROR) << "Could not stat the src: " << from << ", errno=" << strerror(errno);
        return false;
    }

    if ((flag & FLAG_COPY_INSTEADOF_RENAME) == FLAG_COPY_INSTEADOF_RENAME) {
        // 2. use copy
        LOG(DEBUG) << "Use rewrite method...";
        return copy_and_chown(from, to, uid, gid, se_info);
    } else {
        //2. use rename
        LOG(DEBUG) << "Use move method...";
        return rename_and_chown(from, to, uid, gid, se_info);

    }
}

}  // namespace installd
}  // namespace android