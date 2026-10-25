#define LOG_TAG "MiuiMemoryService"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <android_runtime/AndroidRuntime.h>
#include <binder/IPCThreadState.h>
#include <cutils/compiler.h>
#include <linux/errno.h>
#include <log/log.h>
#include <meminfo/procmeminfo.h>
#include <nativehelper/JNIHelp.h>
#include <processgroup/processgroup.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/pidfd.h>
#include <sys/syscall.h>

#include <algorithm>

using android::base::StringPrintf;
using android::base::WriteStringToFile;
using android::meminfo::ProcMemInfo;
using namespace android::meminfo;

#define COMPACT_ACTION_FILE_FLAG 1
#define COMPACT_ACTION_ANON_FLAG 2

using VmaToAdviseFunc = std::function<int(const Vma&)>;
using android::base::unique_fd;

// Legacy method for compacting processes, any new code should
// use compactProcess instead.
static inline void compactProcessProcfs(int pid, const std::string& compactionType) {
    std::string reclaim_path = StringPrintf("/proc/%d/reclaim", pid);
    WriteStringToFile(compactionType, reclaim_path);
}

// Compacts a set of VMAs for pid using an madviseType accepted by process_madvise syscall
// On success returns the total bytes that where compacted. On failure it returns
// a negative error code from the standard linux error codes.
static int64_t compactMemory(const std::vector<Vma>& vmas, int pid, int madviseType) {
    // UIO_MAXIOV is currently a small value and we might have more addresses
    // we do multiple syscalls if we exceed its maximum
    static struct iovec vmasToKernel[UIO_MAXIOV];

    if (vmas.empty()) {
        return 0;
    }

    unique_fd pidfd(pidfd_open(pid, 0));
    if (pidfd < 0) {
        // Skip compaction if failed to open pidfd with any error
        return -errno;
    }

    int64_t totalBytesCompacted = 0;
    for (int iBase = 0; iBase < vmas.size(); iBase += UIO_MAXIOV) {
        int totalVmasToKernel = std::min(UIO_MAXIOV, (int)(vmas.size() - iBase));
        for (int iVec = 0, iVma = iBase; iVec < totalVmasToKernel; ++iVec, ++iVma) {
            vmasToKernel[iVec].iov_base = (void*)vmas[iVma].start;
            vmasToKernel[iVec].iov_len = vmas[iVma].end - vmas[iVma].start;
        }

        auto bytesCompacted =
                process_madvise(pidfd, vmasToKernel, totalVmasToKernel, madviseType, 0);
        if (CC_UNLIKELY(bytesCompacted == -1)) {
            return -errno;
        }

        totalBytesCompacted += bytesCompacted;
    }

    return totalBytesCompacted;
}

static int getFilePageAdvice(const Vma& vma) {
    if (vma.inode > 0 && !vma.is_shared) {
        return MADV_COLD;
    }
    return -1;
}
static int getAnonPageAdvice(const Vma& vma) {
    if (vma.inode == 0 && !vma.is_shared) {
        return MADV_PAGEOUT;
    }
    return -1;
}
static int getAnyPageAdvice(const Vma& vma) {
    if (vma.inode == 0 && !vma.is_shared) {
        return MADV_PAGEOUT;
    }
    return MADV_COLD;
}

// Perform a full process compaction using process_madvise syscall
// reading all filtering VMAs and filtering pages as specified by pageFilter
static int64_t compactProcess(int pid, VmaToAdviseFunc vmaToAdviseFunc) {
    ProcMemInfo meminfo(pid);
    std::vector<Vma> pageoutVmas, coldVmas;
    auto vmaCollectorCb = [&coldVmas,&pageoutVmas,&vmaToAdviseFunc](const Vma& vma) {
        int advice = vmaToAdviseFunc(vma);
        switch (advice) {
            case MADV_COLD:
                coldVmas.push_back(vma);
                break;
            case MADV_PAGEOUT:
                pageoutVmas.push_back(vma);
                break;
        }
    };
    meminfo.ForEachVmaFromMaps(vmaCollectorCb);

    int64_t pageoutBytes = compactMemory(pageoutVmas, pid, MADV_PAGEOUT);
    if (pageoutBytes < 0) {
        // Error, just forward it.
        return pageoutBytes;
    }

    int64_t coldBytes = compactMemory(coldVmas, pid, MADV_COLD);
    if (coldBytes < 0) {
        // Error, just forward it.
        return coldBytes;
    }

    return pageoutBytes + coldBytes;
}

// Compact process using process_madvise syscall or fallback to procfs in
// case syscall does not exist.
static void compactProcessOrFallback(int pid, int compactionFlags) {
    if ((compactionFlags & (COMPACT_ACTION_ANON_FLAG | COMPACT_ACTION_FILE_FLAG)) == 0) return;

    bool compactAnon = compactionFlags & COMPACT_ACTION_ANON_FLAG;
    bool compactFile = compactionFlags & COMPACT_ACTION_FILE_FLAG;

    // Set when the system does not support process_madvise syscall to avoid
    // gathering VMAs in subsequent calls prior to falling back to procfs
    static bool shouldForceProcFs = false;
    std::string compactionType;
    VmaToAdviseFunc vmaToAdviseFunc;

    if (compactAnon) {
        if (compactFile) {
            compactionType = "all";
            vmaToAdviseFunc = getAnyPageAdvice;
        } else {
            compactionType = "anon";
            vmaToAdviseFunc = getAnonPageAdvice;
        }
    } else {
        compactionType = "file";
        vmaToAdviseFunc = getFilePageAdvice;
    }

    if (shouldForceProcFs || compactProcess(pid, vmaToAdviseFunc) == -ENOSYS) {
        shouldForceProcFs = true;
        compactProcessProcfs(pid, compactionType);
    }
}

static void android_os_NativeMiuiMemReclaimer_compactProcess(JNIEnv*, jobject, jint pid,
                                                                    jint compactionFlags) {
    compactProcessOrFallback(pid, compactionFlags);
}

static const JNINativeMethod sMethods[] = {
        /* name, signature, funcPtr */
        {"compactProcess", "(II)V", (void*)android_os_NativeMiuiMemReclaimer_compactProcess}
};

int register_android_os_NativeMiuiMemReclaimer(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "android/os/NativeMiuiMemReclaimer",
                                    sMethods, NELEM(sMethods));
}
