// std
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>

// bionic
#include <ftw.h>
#undef __bitwise  // redefined in ext2fs/ext2_types.h

// e2fsprogs
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>

// cppjson
#include <json/json.h>

// kernel
#include <sys/vfs.h>

// android
#include <cutils/properties.h>
#include <utils/Log.h>

#include "utils.h"
#include "FragInfo.h"
#include "FragInfoRefFile.h"

#define MAX_HIST 16

using namespace std;

// LEGACY CODE

struct free_blocks_info {
    uint64_t free_extents[MAX_HIST];
    uint64_t free_blocks[MAX_HIST];
    uint64_t total_blocks;
};

static uint64_t log2(uint64_t arg) {
    uint64_t idx = 0;

    arg >>= 1;
    while (arg) {
        idx++;
        arg >>= 1;
    }
    return idx;
}

static int open_device(char *device_name, ext2_filsys *fs) {
    int retval;
    int flag = EXT2_FLAG_FORCE | EXT2_FLAG_64BITS;

    retval = ext2fs_open(device_name, flag, 0, 0, unix_io_manager, fs);
    if (retval) {
        ALOGE("error while opening filesystem: %s. error %d", device_name, retval);
        return retval;
    }
    (*fs)->default_bitmap_type = EXT2FS_BMAP64_RBTREE;
    return 0;
}

static void close_device(ext2_filsys fs) {
    int retval = ext2fs_close(fs);

    if (retval)
        ALOGE("error while closing filesystem %d", retval);
}

static void update_free_stats(struct free_blocks_info *info,
        uint64_t free_size) {
    uint64_t idx;

    idx = log2(free_size);
    if (idx >= MAX_HIST)
        idx = MAX_HIST-1;
    info->free_extents[idx]++;
    info->free_blocks[idx] += free_size;
    info->total_blocks += free_size;
}

static int scan_blocks(ext2_filsys fs, struct free_blocks_info *info) {
    uint64_t blocks_count = ext2fs_blocks_count(fs->super);
    uint64_t last_free_size = 0;
    uint64_t blk = fs->super->s_first_data_block;
    int used;

    int ret = ext2fs_read_block_bitmap(fs);
    if (ret) {
        ALOGE("error while read block bitmap,ret %d", ret);
        return ret;
    }

    for (int i = 0; i < MAX_HIST; i++) {
        info->free_extents[i] = 0;
        info->free_blocks[i] = 0;
    }

    while (blk < blocks_count) {
        used = ext2fs_fast_test_block_bitmap2(fs->block_map,
                        blk >> fs->cluster_ratio_bits);
        if (!used) {
            last_free_size++;
        }

        if (used && last_free_size != 0) {
            update_free_stats(info, last_free_size);
            last_free_size = 0;
        }
        blk++;
    }
    if (last_free_size != 0)
        update_free_stats(info, last_free_size);

    return 0;
}

int getFreeFrag(struct free_blocks_info &free_blocks_info) {
    ext2_filsys fs = NULL;
    char device_name[256];
    char data_path[] = "/data";
    int retcode = 0;

    fill(device_name, device_name + 256, 0);

    if ((retcode = get_block_device(data_path, device_name)) != 0) {
        return retcode + 1000;
    }

    if ((retcode = open_device(device_name, &fs)) != 0) {
        return retcode + 2000;
    }

    memset(&free_blocks_info, 0, sizeof(free_blocks_info));
    if ((retcode = scan_blocks(fs, &free_blocks_info)) != 0) {
        close_device(fs);
        return retcode + 3000;
    }
    close_device(fs);
    return 0;
}


#define MAX_DEPTH 16
#define MAX_FILESIZE_HIST 32
#define JSON_FORMAT_VERSION 5  // add 1 after every single format change


static double getElapsedSeconds(chrono::steady_clock::time_point begin,
                                chrono::steady_clock::time_point end) {
    using namespace chrono;
    duration<double> timeSpan = duration_cast<duration<double>>(end - begin);
    return timeSpan.count();
}


static string getDateString(const time_t tt) {
    char buf[9];
    strftime(buf, 9, "%Y%m%d", localtime(&tt));
    return string(buf);
}

static string readFileHeader(string path) {
    char buf[4096];
    fill(buf, buf + 4096, 0);
    ifstream file(path);
    if (file.good()) {
        file.read(buf, 4096);
        file.close();
    }
    return string(buf);
}


template <typename T> static T readFileAs(string path) {
    T ret{};
    ifstream file(path);
    if (file.good()) {
        file >> ret;
        file.close();
    }
    return ret;
}


struct ctimeSample {
    int count = 0;
    bool operator < (const ctimeSample& other) const {
        return count < other.count;
    }
};

static map<string, ctimeSample> ctimeStat;
static map<string, int> ctimeHist;
static unsigned long long dirDepthStat[MAX_DEPTH] = {0};
static unsigned long long dirNumber = 0, fileNumber = 0;
static unsigned long long filesizeStat[MAX_FILESIZE_HIST] = {0};
// [0]: 0-2B      [1]: 2-4B      [2]: 4-8B      [3]: 8-16B
// [4]: 16-32B    [5]: 32-64B    [6]: 64-128B   [7]: 128-256B
// [8]: 256-512B  [9]: 512B-1K   [10]: 1-2K     [11]: 2-4K
// [12]: 4-8K     [13]: 8-16K    [14]: 16-32K   [15]: 32-64K
// [16]: 64-128K  [17]: 128-256K [18]: 256-512K [19]: 512K-1M
// [20]: 1-2M     [21]: 2-4M     [22]: 4-8M     [23]: 8-16M
// [24]: 16-32M   [25]: 32-64M   [26]: 64-128M  [27]: 128-256M
// [28]: 256-512M [29]: 512M-1G  [30]: 1-2G     [31]: 2G+

static int iterFile(const char *path, const struct stat64 *st,
        int fnflag, struct FTW *ftw) {
    (void)path;

    // ctime
    string dateString = getDateString(st->st_ctime);
    if (dateString >= "20091231") {
        ctimeStat[dateString].count += 1;
        ctimeHist[dateString.substr(0, 6)] += 1;
    }

    if (fnflag == FTW_F) {
        fileNumber += 1;

        // filesize
        uint64_t logIndex = log2(st->st_size);
        if (logIndex < MAX_FILESIZE_HIST) {
            filesizeStat[logIndex] += 1;
        } else {
            filesizeStat[MAX_FILESIZE_HIST - 1] += 1;
        }

    } else if (fnflag == FTW_D) {
        dirNumber += 1;

        // depth
        if (ftw->level >= 0 && ftw->level < MAX_DEPTH) {
            dirDepthStat[ftw->level] += 1;
        }
    }

    return 0;
}


string getFragInfo() {
    Json::Value root, dp;  // dp - data partition
    string content, path;

    auto beginTime = chrono::steady_clock::now();

    // get /data dev name
    char devPathBuf[1024];
    fill(devPathBuf, devPathBuf + 1024, 0);
    static char dataPath[] = "/data";
    get_block_device(dataPath, devPathBuf);

    string dpDevPath = devPathBuf;
    string dpDevName = dpDevPath.substr(
            dpDevPath.find_last_of('/') + 1,
            dpDevPath.length()
    );
    dp["dev_name"] = dpDevName;

    // ext4 fs write bytes
    dp["lifetime_write_kbytes"] = readFileAs<unsigned long long>(
            "/sys/fs/ext4/" + dpDevName + "/lifetime_write_kbytes");

    // emmc life time
    root["life_time_est_typ_a"] = readFileHeader(
            "/sys/class/block/mmcblk0/device/life_time_est_typ_a");
    root["life_time_est_typ_b"] = readFileHeader(
            "/sys/class/block/mmcblk0/device/life_time_est_typ_b");

    // ufs life time
    root["dump_health_desc"] = readFileHeader(
            "/sys/kernel/debug/ufshcd0/dump_health_desc");

    // write json format version
    root["json_format_version"] = JSON_FORMAT_VERSION;

    // free frag (e2freefrag)
    struct free_blocks_info fbi;
    memset(&fbi, 0, sizeof(struct free_blocks_info));
    int ret = getFreeFrag(fbi);
    if (ret == 0) {
        for (int i = 0; i < MAX_HIST; i++) {
            Json::Value v;
            v["extent_nr"] = (Json::UInt64)fbi.free_extents[i];
            v["block_nr"] = (Json::UInt64)fbi.free_blocks[i];
            dp["freefrag"].append(v);
        }
    }
    dp["freefrag_retcode"] = ret;

    // statfs data partition
    struct statfs sf;
    if (statfs(dataPath, &sf) == 0) {
        dp["statfs"]["f_type"] = (Json::UInt64)sf.f_type;
        dp["statfs"]["f_bsize"] = (Json::UInt64)sf.f_bsize;
        dp["statfs"]["f_blocks"] = (Json::UInt64)sf.f_blocks;
        dp["statfs"]["f_bfree"] = (Json::UInt64)sf.f_bfree;
        dp["statfs"]["f_bavail"] = (Json::UInt64)sf.f_bavail;
        dp["statfs"]["f_files"] = (Json::UInt64)sf.f_files;
        dp["statfs"]["f_ffree"] = (Json::UInt64)sf.f_ffree;
        dp["statfs"]["f_namelen"] = (Json::UInt64)sf.f_namelen;
        dp["statfs"]["f_frsize"] = (Json::UInt64)sf.f_frsize;
        dp["statfs"]["f_flags"] = (Json::UInt64)sf.f_flags;
    }

    // walk on /data partition
    ctimeStat.clear();
    ctimeHist.clear();
    fill(dirDepthStat, dirDepthStat + MAX_DEPTH, 0);
    dirNumber = 0;
    fileNumber = 0;
    fill(filesizeStat, filesizeStat + MAX_FILESIZE_HIST, 0);
    // XXX: The third param have not implemented in bionic yet.
    nftw64("/data", iterFile, 1, FTW_PHYS | FTW_MOUNT);

    // fileNumber and dirNumber
    dp["file_nr"] = fileNumber;
    dp["dir_nr"] = dirNumber;

    // ctimeStat
    vector<pair<string, ctimeSample>> vec(ctimeStat.begin(), ctimeStat.end());
    partial_sort(
        vec.begin(),
        vec.size() >= 10 ? vec.begin() + 10 : vec.end(),
        vec.end()
    );
    for (size_t i = 0; i < min(size_t(10), vec.size()); i++) {
        dp["ctime"][vec[i].first] = vec[i].second.count;
    }

    // ctimeHist
    for (auto pr : ctimeHist) {
        dp["ctimeHist"][pr.first] = pr.second;
    }

    // ctimeRefFile
    for (string refFile : refFiles) {
        struct stat st;
        if (stat(refFile.c_str(), &st) == 0) {
            dp["ctime_reffile"][refFile] = getDateString(st.st_ctime);
        } else {
            dp["ctime_reffile"][refFile] = Json::nullValue;
        }
    }

    // dirDepthStat
    for (auto i : dirDepthStat) {
        dp["dir_depth"].append(i);
    }

    // filesizeStat
    for (auto i : filesizeStat) {
        dp["filesize"].append(i);
    }

    // defrag cloud control properties and android sdk version
    char buffer[4][PROPERTY_VALUE_MAX];
    fill(&buffer[0][0], &buffer[0][0] + sizeof(buffer), 0);
    property_get("persist.sys.mem_cc", buffer[0], "");
    property_get("persist.sys.mem_cgated", buffer[1], "");
    property_get("persist.sys.mem_fgated", buffer[2], "");
    property_get("ro.build.version.sdk", buffer[3], "");

    Json::Value prop;
    prop["persist.sys.mem_cc"] = buffer[0];
    prop["persist.sys.mem_cgated"] = buffer[1];
    prop["persist.sys.mem_fgated"] = buffer[2];
    prop["ro.build.version.sdk"] = buffer[3];
    root["property"] = prop;

    // time count
    auto endTime = chrono::steady_clock::now();
    root["elapsed_second"] = getElapsedSeconds(beginTime, endTime);

    // easy to remove duplicated data in ETL
    string uuid = readFileHeader("/proc/sys/kernel/random/uuid");
    uuid.erase(remove(uuid.begin(), uuid.end(), '\n'));
    root["uuid"] = uuid;

    root["data"] = dp;

    string jsonString = Json::FastWriter().write(root);
    jsonString.erase(remove(jsonString.begin(), jsonString.end(), '\n'));
    return jsonString;
}


void printFragInfo() {
    cout << getFragInfo() << endl;
}

