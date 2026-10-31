#ifndef LOG_TAG
#define LOG_TAG "MIUIScout Memory"
#endif

#include "jni.h"
#include "core_jni_helpers.h"
#include <stdio.h>
#include <string.h>
#include "utils/Log.h"
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <android-base/stringprintf.h>
#include <android-base/file.h>
#include <dmabufinfo/dmabufinfo.h>
#include <dmabufinfo/dmabuf_sysfs_stats.h>

#define NATIVE_ADJ (-1000)
#define SYSTEM_ADJ (-900)
#define UNKNOWN_ADJ (1001)
#define INVALID_ADJ (-10000)
#define KGSL_PAGE_ALLOC_PATH "/sys/class/kgsl/kgsl/page_alloc"

using namespace android;
using android::base::ReadFileToString;
using DmaBuffer = ::android::dmabufinfo::DmaBuffer;

static struct usageinfo_offsets_t {
    jclass classObject;
    jmethodID constructor;
    jmethodID add;
    jmethodID setTotalSize;
    jmethodID setKernelRss;
    jmethodID setTotalRss;
    jmethodID setTotalPss;
    jmethodID setRuntimeTotalSize;
    jmethodID setNativeTotalSize;
} gUsageInfoOffsets;

static struct procusageinfo_offsets_t {
    jclass classObject;
    jmethodID constructor;
    jmethodID setName;
    jmethodID setPid;
    jmethodID setOomadj;
    jmethodID setRss;
    jmethodID setPss;
} gProcUsageInfoOffsets;

static struct kgslinfo_offsets_t {
    jclass classObject;
    jmethodID constructor;
    jmethodID add;
    jmethodID setTotalSize;
    jmethodID setRuntimeTotalSize;
    jmethodID setNativeTotalSize;
} gKgslInfoOffsets;

static struct prockgslinfo_offsets_t {
    jclass classObject;
    jmethodID constructor;
    jmethodID setName;
    jmethodID setPid;
    jmethodID setOomadj;
    jmethodID setRss;
    jmethodID setGfxDev;
    jmethodID setGlMtrack;
} gProcKgslInfoOffsets;

static std::string GetProcessComm(const pid_t pid) {
    std::string pid_path = android::base::StringPrintf("/proc/%d/cmdline", pid);
    std::ifstream in{pid_path};
    if (!in) return std::string("N/A");
    std::string line;
    std::getline(in, line);
    if (!in) return std::string("N/A");
    return line;
}

static int GetPidOomadj(const pid_t pid) {
    int oomadj;
    std::string adj_result;
    std::string adj_path = "/proc/" + std::to_string(pid) + "/oom_score_adj";
    if (!ReadFileToString(adj_path, &adj_result, true)) {
        ALOGE("Failed to read %s \n", adj_path.c_str());
        return INVALID_ADJ;
    }
    oomadj = std::stoi(adj_result);
    if (oomadj >= UNKNOWN_ADJ || oomadj < NATIVE_ADJ) {
        ALOGE("unknown oomadj reset to %d \n", INVALID_ADJ);
        return INVALID_ADJ;
    }
    return oomadj;
}

static long GetPidGfxDev(const pid_t pid) {
    long gfx_dev_size;
    std::string size;
    std::string path = "/sys/class/kgsl/kgsl/proc/" + std::to_string(pid) + "/gpumem_mapped";
    if (!ReadFileToString(path, &size, true)) {
        ALOGE("Failed to read %s \n", path.c_str());
        return 0;
    }
    gfx_dev_size = std::stol(size);
    if (gfx_dev_size < 0) {
        ALOGE("pid(%d) gpumem_mapped size is invalid, reset to %d \n", pid, 0);
        return 0;
    }
    return gfx_dev_size;
}

static long GetPidGlMtrack(const pid_t pid) {
    long gl_mtrack_size;
    std::string size;
    std::string path = "/sys/class/kgsl/kgsl/proc/" + std::to_string(pid) + "/gpumem_unmapped";
    if (!ReadFileToString(path, &size, true)) {
        ALOGE("Failed to read %s \n", path.c_str());
        return 0;
    }
    gl_mtrack_size = std::stol(size);
    if (gl_mtrack_size < 0) {
        ALOGE("pid(%d) gpumem_unmapped size is invalid, reset to %d \n", pid, 0);
        return 0;
    }
    return gl_mtrack_size;
}

static long GetTotalKgsl() {
    long total_size;
    std::string size;
    if (!ReadFileToString(std::string(KGSL_PAGE_ALLOC_PATH), &size, true)) {
        ALOGE("Failed to read %s \n", KGSL_PAGE_ALLOC_PATH);
        return 0;
    }
    total_size = std::stol(size);
    if (total_size < 0) {
        ALOGE("kgsl size is invalid, reset to %d \n", 0);
        return 0;
    }
    return total_size;
}

static bool ReadMiuiDmaBufs(std::vector<DmaBuffer>* bufs) {
    bufs->clear();

    std::unique_ptr<DIR, int (*)(DIR*)> dir(opendir("/proc"), closedir);
    if (!dir) {
        ALOGE("Failed to open /proc directory\n");
        bufs->clear();
        return false;
    }

    struct dirent* dent;
    while ((dent = readdir(dir.get()))) {
        if (dent->d_type != DT_DIR) continue;

        int pid = atoi(dent->d_name);
        if (pid == 0) {
            continue;
        }

        if (!ReadDmaBufFdRefs(pid, bufs)) {
            ALOGE("Failed to read dmabuf fd references for pid %d\n", pid);
            continue;
        }

        if (!ReadDmaBufMapRefs(pid, bufs)) {
            ALOGE("Failed to read dmabuf map references for pid %d\n", pid);
            continue;
        }
    }

    return true;
}

static jobject com_miui_server_stability_ScoutDisplayMemoryManager_readDmabufInfo(JNIEnv *env, jobject clazz) {
    std::vector<DmaBuffer> bufs;
    if (!ReadMiuiDmaBufs(&bufs)) {
        ALOGE("Read DmaBufs Fail");
        return nullptr;
    }
    if (bufs.empty()) {
        ALOGE("dmabuf info not found");
        return nullptr;
    }
    jobject dmabufInfo = env->NewObject(gUsageInfoOffsets.classObject, gUsageInfoOffsets.constructor);
    std::unordered_map<pid_t, std::set<ino_t>> pid_to_inodes = {};
    uint64_t total_size = 0;  // Total size of dmabufs in the system
    uint64_t kernel_rss = 0;  // Total size of dmabufs NOT mapped or opened by a process
    uint64_t native_rss = 0;
    uint64_t runtime_rss = 0;
    for (auto& buf : bufs) {
        for (auto pid : buf.pids()) {
            pid_to_inodes[pid].insert(buf.inode());
        }
        total_size += buf.size();
        if (buf.fdrefs().empty() && buf.maprefs().empty()) {
            kernel_rss += buf.size();
        }
    }
    // Create an inode to dmabuf map. We know inodes are unique..
    std::unordered_map<ino_t, DmaBuffer> inode_to_dmabuf;
    for (auto buf : bufs) {
        inode_to_dmabuf[buf.inode()] = buf;
    }
    uint64_t total_rss = 0, total_pss = 0;
    for (auto& [pid, inodes] : pid_to_inodes) {
        uint64_t pss = 0;
        uint64_t rss = 0;
        for (auto& inode : inodes) {
            DmaBuffer& buf = inode_to_dmabuf[inode];
            uint64_t proc_pss = buf.Pss(pid);
            rss += buf.size();
            pss += proc_pss;
        }
        int adj = GetPidOomadj(pid);
        std::string pid_name = GetProcessComm(pid);
        jobject dmabufProcInfo = env->NewObject(gProcUsageInfoOffsets.classObject, gProcUsageInfoOffsets.constructor);
        jstring name= env->NewStringUTF(pid_name.c_str());
        env->CallObjectMethod(dmabufProcInfo, gProcUsageInfoOffsets.setName, name);
        env->CallObjectMethod(dmabufProcInfo, gProcUsageInfoOffsets.setPid, pid);
        env->CallObjectMethod(dmabufProcInfo, gProcUsageInfoOffsets.setOomadj, adj);
        env->CallObjectMethod(dmabufProcInfo, gProcUsageInfoOffsets.setRss, static_cast<jlong>(rss / 1024));
        env->CallObjectMethod(dmabufProcInfo, gProcUsageInfoOffsets.setPss, static_cast<jlong>(pss / 1024));
        env->CallObjectMethod(dmabufInfo, gUsageInfoOffsets.add, dmabufProcInfo);
        env->DeleteLocalRef(dmabufProcInfo);
        env->DeleteLocalRef(name);
        total_rss += rss;
        total_pss += pss;
        if (adj > NATIVE_ADJ && adj < UNKNOWN_ADJ) {
            runtime_rss += rss;
        } else {
            native_rss += rss;
        }
    }
    env->CallObjectMethod(dmabufInfo, gUsageInfoOffsets.setTotalSize, static_cast<jlong>(total_size / 1024));
    env->CallObjectMethod(dmabufInfo, gUsageInfoOffsets.setKernelRss, static_cast<jlong>(kernel_rss / 1024));
    env->CallObjectMethod(dmabufInfo, gUsageInfoOffsets.setTotalRss, static_cast<jlong>(total_rss / 1024));
    env->CallObjectMethod(dmabufInfo, gUsageInfoOffsets.setTotalPss, static_cast<jlong>(total_pss / 1024));
    env->CallObjectMethod(dmabufInfo, gUsageInfoOffsets.setRuntimeTotalSize, static_cast<jlong>(runtime_rss / 1024));
    env->CallObjectMethod(dmabufInfo, gUsageInfoOffsets.setNativeTotalSize, static_cast<jlong>(native_rss / 1024));
    return dmabufInfo;
}

static jlong com_miui_server_stability_ScoutDisplayMemoryManager_getTotalKgsl(JNIEnv *env, jobject clazz) {
    long totalSize = GetTotalKgsl();
    return static_cast<jlong>(totalSize / 1024);
}

static jobject com_miui_server_stability_ScoutDisplayMemoryManager_readKgslInfo(JNIEnv *env, jobject clazz) {
    std::unique_ptr<DIR, int (*)(DIR*)> dir(opendir("/sys/class/kgsl/kgsl/proc"), closedir);
    if (!dir) {
        ALOGE("Failed to open /sys/class/kgsl/kgsl/proc directory\n");
        return nullptr;
    }
    jobject kgslInfo = env->NewObject(gKgslInfoOffsets.classObject, gKgslInfoOffsets.constructor);
    struct dirent* dent;
    uint64_t total_size = 0;
    uint64_t runtime_size = 0;
    uint64_t native_size = 0;
    while ((dent = readdir(dir.get()))) {
        if (dent->d_type != DT_DIR) continue;

        int pid = atoi(dent->d_name);
        if (pid == 0) {
            continue;
        }
        long gfx_dev_size = GetPidGfxDev(pid);
        long gl_mtrack_size = GetPidGlMtrack(pid);
        long pid_total_size = gfx_dev_size + gl_mtrack_size;
        if (pid_total_size > 0) {
            int adj = GetPidOomadj(pid);
            std::string pid_name = GetProcessComm(pid);
            jobject kgslProcInfo = env->NewObject(gProcKgslInfoOffsets.classObject, gProcKgslInfoOffsets.constructor);
            jstring name = env->NewStringUTF(pid_name.c_str());
            env->CallObjectMethod(kgslProcInfo, gProcKgslInfoOffsets.setName, name);
            env->CallObjectMethod(kgslProcInfo, gProcKgslInfoOffsets.setPid, pid);
            env->CallObjectMethod(kgslProcInfo, gProcKgslInfoOffsets.setOomadj, adj);
            env->CallObjectMethod(kgslProcInfo, gProcKgslInfoOffsets.setRss, static_cast<jlong>(pid_total_size / 1024));
            env->CallObjectMethod(kgslProcInfo, gProcKgslInfoOffsets.setGfxDev, static_cast<jlong>(gfx_dev_size / 1024));
            env->CallObjectMethod(kgslProcInfo, gProcKgslInfoOffsets.setGlMtrack, static_cast<jlong>(gl_mtrack_size / 1024));
            env->CallObjectMethod(kgslInfo, gKgslInfoOffsets.add, kgslProcInfo);
            env->DeleteLocalRef(kgslProcInfo);
            env->DeleteLocalRef(name);
            if (adj > NATIVE_ADJ && adj < UNKNOWN_ADJ) {
                runtime_size += pid_total_size;
            } else {
                native_size += pid_total_size;
            }
            total_size += pid_total_size;
        }
    }
    env->CallObjectMethod(kgslInfo, gKgslInfoOffsets.setTotalSize, static_cast<jlong>(total_size / 1024));
    env->CallObjectMethod(kgslInfo, gKgslInfoOffsets.setRuntimeTotalSize, static_cast<jlong>(runtime_size / 1024));
    env->CallObjectMethod(kgslInfo, gKgslInfoOffsets.setNativeTotalSize, static_cast<jlong>(native_size / 1024));
    return kgslInfo;
}

static const JNINativeMethod methods[] = {
    {"readDmabufInfo",   "()Lcom/miui/server/stability/DmaBufUsageInfo;", (void *) com_miui_server_stability_ScoutDisplayMemoryManager_readDmabufInfo},
    {"getTotalKgsl",   "()J", (void *) com_miui_server_stability_ScoutDisplayMemoryManager_getTotalKgsl},
    {"readKgslInfo",   "()Lcom/miui/server/stability/KgslUsageInfo;", (void *) com_miui_server_stability_ScoutDisplayMemoryManager_readKgslInfo},
};

static const char* const kClassPathName = "com/miui/server/stability/ScoutDisplayMemoryManager";

jboolean register_com_miui_server_stability_ScoutDisplayMemoryManager(JNIEnv* env) {
    jclass procUsageInfoClass = FindClassOrDie(env, "com/miui/server/stability/DmaBufProcUsageInfo");
    gProcUsageInfoOffsets.classObject = MakeGlobalRefOrDie(env, procUsageInfoClass);
    gProcUsageInfoOffsets.constructor = GetMethodIDOrDie(env, gProcUsageInfoOffsets.classObject, "<init>", "()V");
    gProcUsageInfoOffsets.setName =
        GetMethodIDOrDie(env, gProcUsageInfoOffsets.classObject, "setName", "(Ljava/lang/String;)V");
    gProcUsageInfoOffsets.setPid =
        GetMethodIDOrDie(env, gProcUsageInfoOffsets.classObject, "setPid", "(I)V");
    gProcUsageInfoOffsets.setOomadj =
        GetMethodIDOrDie(env, gProcUsageInfoOffsets.classObject, "setOomadj", "(I)V");
    gProcUsageInfoOffsets.setRss =
        GetMethodIDOrDie(env, gProcUsageInfoOffsets.classObject, "setRss", "(J)V");
    gProcUsageInfoOffsets.setPss =
        GetMethodIDOrDie(env, gProcUsageInfoOffsets.classObject, "setPss", "(J)V");

    jclass usageInfoClass = FindClassOrDie(env, "com/miui/server/stability/DmaBufUsageInfo");
    gUsageInfoOffsets.classObject = MakeGlobalRefOrDie(env, usageInfoClass);
    gUsageInfoOffsets.constructor = GetMethodIDOrDie(env, gUsageInfoOffsets.classObject, "<init>", "()V");
    gUsageInfoOffsets.add =
        GetMethodIDOrDie(env, gUsageInfoOffsets.classObject, "add", "(Lcom/miui/server/stability/DmaBufProcUsageInfo;)V");
    gUsageInfoOffsets.setTotalSize =
        GetMethodIDOrDie(env, gUsageInfoOffsets.classObject, "setTotalSize", "(J)V");
    gUsageInfoOffsets.setKernelRss =
        GetMethodIDOrDie(env, gUsageInfoOffsets.classObject, "setKernelRss", "(J)V");
    gUsageInfoOffsets.setTotalRss =
        GetMethodIDOrDie(env, gUsageInfoOffsets.classObject, "setTotalRss", "(J)V");
    gUsageInfoOffsets.setTotalPss =
        GetMethodIDOrDie(env, gUsageInfoOffsets.classObject, "setTotalPss", "(J)V");
    gUsageInfoOffsets.setRuntimeTotalSize =
        GetMethodIDOrDie(env, gUsageInfoOffsets.classObject, "setRuntimeTotalSize", "(J)V");
    gUsageInfoOffsets.setNativeTotalSize =
        GetMethodIDOrDie(env, gUsageInfoOffsets.classObject, "setNativeTotalSize", "(J)V");

    jclass procKgslInfoClass = FindClassOrDie(env, "com/miui/server/stability/KgslProcUsageInfo");
    gProcKgslInfoOffsets.classObject = MakeGlobalRefOrDie(env, procKgslInfoClass);
    gProcKgslInfoOffsets.constructor = GetMethodIDOrDie(env, gProcKgslInfoOffsets.classObject, "<init>", "()V");
    gProcKgslInfoOffsets.setName =
        GetMethodIDOrDie(env, gProcKgslInfoOffsets.classObject, "setName", "(Ljava/lang/String;)V");
    gProcKgslInfoOffsets.setPid =
        GetMethodIDOrDie(env, gProcKgslInfoOffsets.classObject, "setPid", "(I)V");
    gProcKgslInfoOffsets.setOomadj =
        GetMethodIDOrDie(env, gProcKgslInfoOffsets.classObject, "setOomadj", "(I)V");
    gProcKgslInfoOffsets.setRss =
            GetMethodIDOrDie(env, gProcKgslInfoOffsets.classObject, "setRss", "(J)V");
    gProcKgslInfoOffsets.setGfxDev =
        GetMethodIDOrDie(env, gProcKgslInfoOffsets.classObject, "setGfxDev", "(J)V");
    gProcKgslInfoOffsets.setGlMtrack =
        GetMethodIDOrDie(env, gProcKgslInfoOffsets.classObject, "setGlMtrack", "(J)V");

    jclass kgslInfoClass = FindClassOrDie(env, "com/miui/server/stability/KgslUsageInfo");
    gKgslInfoOffsets.classObject = MakeGlobalRefOrDie(env, kgslInfoClass);
    gKgslInfoOffsets.constructor = GetMethodIDOrDie(env, gKgslInfoOffsets.classObject, "<init>", "()V");
    gKgslInfoOffsets.add =
        GetMethodIDOrDie(env, gKgslInfoOffsets.classObject, "add", "(Lcom/miui/server/stability/KgslProcUsageInfo;)V");
    gKgslInfoOffsets.setTotalSize =
        GetMethodIDOrDie(env, gKgslInfoOffsets.classObject, "setTotalSize", "(J)V");
    gKgslInfoOffsets.setRuntimeTotalSize =
        GetMethodIDOrDie(env, gKgslInfoOffsets.classObject, "setRuntimeTotalSize", "(J)V");
    gKgslInfoOffsets.setNativeTotalSize =
        GetMethodIDOrDie(env, gKgslInfoOffsets.classObject, "setNativeTotalSize", "(J)V");

    jclass clazz = env->FindClass(kClassPathName);
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
