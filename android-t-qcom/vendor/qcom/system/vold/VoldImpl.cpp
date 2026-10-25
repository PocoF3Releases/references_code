#include "VoldImpl.h"
#include "Cld.h"
using android::OK;

extern int master_key_saved;
extern char* password;
extern int password_expiry_time;
extern int password_max_age_seconds;
extern struct crypt_persist_data* persist_data;

static const char* kMvPath = "/system/bin/mv";
static const char* kRmPath = "/system/bin/rm";
static const char* kPropBlockingExec = "persist.sys.blocking_exec";

namespace android {
namespace vold {

#define CONSTRAIN(amount, low, high) \
    ((amount) < (low) ? (low) : ((amount) > (high) ? (high) : (amount)))

bool VoldImpl::pushBackContents(const std::string& path, std::vector<std::string>& cmd,
                             int searchLevels) {
    if (searchLevels == 0) {
        cmd.emplace_back(path);
        return true;
    }
    auto dirp = std::unique_ptr<DIR, int (*)(DIR*)>(opendir(path.c_str()), closedir);
    if (!dirp) {
        PLOG(ERROR) << "Unable to open directory: " << path;
        return false;
    }
    bool found = false;
    struct dirent* ent;
    while ((ent = readdir(dirp.get())) != NULL) {
        if ((!strcmp(ent->d_name, ".")) || (!strcmp(ent->d_name, ".."))) {
            continue;
        }
        auto subdir = path + "/" + ent->d_name;
        found |= pushBackContents(subdir, cmd, searchLevels - 1);
    }
    return found;
}

status_t VoldImpl::execRm(const std::string& path, int startProgress, int stepProgress,
                       const android::sp<android::os::IVoldTaskListener>& listener) {
    notifyProgress(startProgress, listener);

    uint64_t expectedBytes = GetTreeBytes(path);
    uint64_t startFreeBytes = GetFreeBytes(path);

    std::vector<std::string> cmd;
    cmd.push_back(kRmPath);
    cmd.push_back("-f"); /* force: remove without confirmation, no error if it doesn't exist */
    cmd.push_back("-R"); /* recursive: remove directory contents */
    if (!pushBackContents(path, cmd, 2)) {
        LOG(WARNING) << "No contents in " << path;
        return OK;
    }

    if (android::base::GetBoolProperty(kPropBlockingExec, false)) {
        return ForkExecvp(cmd);
    }

    pid_t pid = ForkExecvpAsync(cmd);
    if (pid == -1) return -1;

    int status;
    while (true) {
        if (waitpid(pid, &status, WNOHANG) == pid) {
            if (WIFEXITED(status)) {
                LOG(DEBUG) << "Finished rm with status " << WEXITSTATUS(status);
                return (WEXITSTATUS(status) == 0) ? OK : -1;
            } else {
                break;
            }
        }

        sleep(1);
        uint64_t deltaFreeBytes = GetFreeBytes(path) - startFreeBytes;
        notifyProgress(
            startProgress +
                CONSTRAIN((int)((deltaFreeBytes * stepProgress) / expectedBytes), 0, stepProgress),
            listener);
    }
    return -1;
}

void VoldImpl::notifyProgress(int progress,
                           const android::sp<android::os::IVoldTaskListener>& listener) {
    if (listener) {
        android::os::PersistableBundle extras;
        listener->onStatus(progress, extras);
    }
}

status_t VoldImpl::execMv(const std::string& from, const std::string& to, int startProgress,
                       int stepProgress,
                       const android::sp<android::os::IVoldTaskListener>& listener, int flag /*1-sync, 0-async*/) {
    notifyProgress(startProgress, listener);

    std::vector<std::string> cmd;
    cmd.push_back(kMvPath);
    cmd.push_back(from.c_str());
    cmd.push_back(to.c_str());

    int flagSync = 1;
    if ((flag & flagSync) == flagSync) {
        LOG(DEBUG) << "ForkExecvp sync... ";
        return ForkExecvp(cmd);
    }

    LOG(DEBUG) << "ForkExecvp async... ";
    pid_t pid = ForkExecvpAsync(cmd);
    if (pid == -1) return -1;

    int status;
    while (true) {
        if (waitpid(pid, &status, WNOHANG) == pid) {
            if (WIFEXITED(status)) {
                LOG(DEBUG) << "Finished mv with status " << WEXITSTATUS(status);
                return (WEXITSTATUS(status) == 0) ? OK : -1;
            } else {
                break;
            }
        }
        sleep(1);
        LOG(DEBUG) << "ForkExecvp async: wait... ";
    }
    LOG(ERROR) << "Finished mv with -1 finally!";
    return -1;
}

status_t VoldImpl::moveStorageQ(const std::string& from, const std::string& to,
                                    const android::sp<android::os::IVoldTaskListener>& listener, int flag) {
    // Step 1: clean up any stale data
    if (execRm(to, 10, 10, listener) != OK) {
        LOG(ERROR) << "rm failed!";
        return -1;
    }
    // Step 2: perform actual move
    if (execMv(from, to, 20, 60, listener, flag) != OK) {
        LOG(ERROR) << "move failed!";
        return -1;
    }
    return OK;
}

void VoldImpl::MoveStorage(const std::string& from, const std::string& to,
                 const android::sp<android::os::IVoldTaskListener>& listener, const char *wake_lock, int flag) {
    //android::wakelock::WakeLock wl{kWakeLock};
    auto wl = android::wakelock::WakeLock::tryGet(wake_lock);
    LOG(DEBUG) << "moveStorage...... START, flag=" << flag;

    android::os::PersistableBundle extras;
    status_t res = moveStorageQ(from, to, listener, flag);
    if(listener) {
        listener->onFinished(res, extras);
    }
    LOG(DEBUG) << "moveStorage...... END, target=" << to;
}

void VoldImpl::fixupAppDirRecursiveNative(const std::string& path, int32_t appUid, int32_t gid, int32_t flag) {
    LOG(DEBUG) << "fixupAppDirR:path=" << path << ",uid=" << appUid << ",gid=" << gid << ",flag=" << flag;
    std::thread([=]() { VolumeManager::Instance()->fixupAppDirRecursive(path, appUid, gid, flag); }).detach();
}

void VoldImpl::moveStorageQuickly(const std::string& from, const std::string& to, const android::sp<android::os::IVoldTaskListener>& listener,
                    int32_t flag, int32_t* _aidl_return) {
    *_aidl_return = 0;
    std::thread([=]() { android::vold::MoveStorage(from, to, listener, flag); }).detach();
}

// support fixup app dir recursive, enhance for #fixupAppDir
int VoldImpl::fixupAppDirRecursive(const std::string& path, int32_t appUid, int32_t gid,
        int32_t flag /*bit0-sync(1)/async(0), bit1-not chmod(1)/chmod(0)*/) {
   if(appUid < -1) {
       LOG(ERROR) << "fixupAppDirR - invalid appUid < -1!";
       return -1;
   }
   if (IsSdcardfsUsed()) {
       //sdcardfs magically does this for us
       return OK;
   }

   //1. check binary exists
   std::string chown = "/system/bin/chown";
   if (access(chown.c_str(), F_OK) != 0) {
       LOG(ERROR) << "fixupAppDirR - check chown function, skip default!";
       chown = "/bin/chown";
       if (access(chown.c_str(), F_OK) != 0) {
           LOG(ERROR) << "fixupAppDirR - failed: no chown function.";
           return -1;
       }
   }

   //2. prepare the value
   uid_t tUid = appUid;
   gid_t tGid = gid;
   if(gid < -1) {
        tGid = 1078; //default #AID_EXT_DATA_RW;
   }
   char ug[PATH_MAX];
   snprintf(ug, PATH_MAX, "%d:%d", tUid, tGid);

   //3. execute the cmd
   std::vector<std::string> cmd;
   cmd.push_back(chown);
   cmd.push_back("-R");
   cmd.push_back(ug);
   cmd.push_back(path);

   int32_t flagSync = 1;
   if ((flag & flagSync) == flagSync) {
       LOG(WARNING) << "fixupAppDirR - Chown sync...";
       return android::vold::ForkExecvp(cmd);
   }

   LOG(WARNING) << "fixupAppDirR - Chown async...";
   pid_t pid = android::vold::ForkExecvpAsync(cmd);
   if (pid == -1) {
       LOG(WARNING) << "fixupAppDirR - fail to execute!, errno=" << strerror(errno);
       return -1;
   }

   int status;
   int ret = OK;
   while (true) {
       if (waitpid(pid, &status, WNOHANG) == pid) {
           if (WIFEXITED(status)) {
               LOG(DEBUG) << "Finished fixup with status " << WEXITSTATUS(status);
               ret = (WEXITSTATUS(status) == 0) ? OK : -1;
           }
           break;
       }
       sleep(1);
       LOG(DEBUG) << "fixupAppDirR async: wait... ";
   }

   //4. check we need chmod as a part of fixup
   int32_t flagNoChmod = 1 << 1;
   if((flag & flagNoChmod) == flagNoChmod) {
       LOG(WARNING) << "fixupAppDirR async - end: no need to fixup mode... ";
       return ret;
   }

   std::string chmod = "/system/bin/chmod";
   if (access(chmod.c_str(), F_OK) != 0) {
       LOG(ERROR) << "fixupAppDirR - check chmod function, skip default!";
       chmod = "/bin/chmod";
       if (access(chmod.c_str(), F_OK) != 0) {
           LOG(ERROR) << "fixupAppDirR - failed: no chmod function.";
           return -1;
       }
   }

   //4.1. execute the cmdx
   std::vector<std::string> cmdx;
   cmdx.push_back(chmod);
   cmdx.push_back("-R");
   cmdx.push_back("770"); //mode = 770
   cmdx.push_back(path);

   LOG(WARNING) << "fixupAppDirR - chmod async...";
   pid = android::vold::ForkExecvpAsync(cmdx); //we only support async
   if (pid == -1) {
       LOG(WARNING) << "fixupAppDirR - fail to execute!, errno=" << strerror(errno);
       return -1;
   }
   while (true) {
       if (waitpid(pid, &status, WNOHANG) == pid) {
           if (WIFEXITED(status)) {
               LOG(DEBUG) << "Finished fixup mode with status " << WEXITSTATUS(status);
               ret = (WEXITSTATUS(status) == 0) ? OK : -1;
           }
           break;
       }
       sleep(1);
       LOG(DEBUG) << "fixupAppDirR async: fix mode wait ... ";
   }

   LOG(WARNING) << "fixupAppDirR - end...";
   return ret;
}
// cld fearure
void VoldImpl::SetCldListener(const android::sp<android::os::IVoldTaskListener>& listener) {
    miuiSetCldListener(listener);
}

int VoldImpl::GetCldFragLevel() {
    int re = miuiGetFragLevelHal();
    return miuiGetFragLevelHal();
}

int VoldImpl::RunCldOnHal() {
    return miuiRunCldOnHal();
}
// end
extern "C" IVoldStub* create() {
    return new VoldImpl;
}

extern "C" void destroy(IVoldStub* impl) {
    delete impl;
}

}
}
