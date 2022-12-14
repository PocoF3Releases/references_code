/*
 * Copyright (C) Xiaomi Inc.
 *
 */
#define LOG_TAG "mqsasd"

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <string.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <private/android_filesystem_config.h>

#include <string>
#include "cksum.h"
#include "utils.h"

const static unsigned long FILE_COPY_SIZE = 262144;

pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int g_stop_flag=0;

int get_block_device(char *path, char *block) {
    FILE *fp = fopen("/proc/mounts", "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            int fsname0, fsname1, dir0, dir1, type0, type1, opts0, opts1, mnt_freq, mnt_passno;
            if (sscanf(line, " %n%*s%n %n%*s%n %n%*s%n %n%*s%n %d %d",
                    &fsname0, &fsname1, &dir0, &dir1, &type0, &type1, &opts0, &opts1,
                    &mnt_freq, &mnt_passno) == 2) {
                char *mnt_fsname = &line[fsname0];
                line[fsname1] = '\0';
                char *mnt_dir = &line[dir0];
                line[dir1] = '\0';

                if (strncmp(path, mnt_dir, strlen(path)) == 0 &&
                    strncmp(mnt_fsname, "tmpfs", strlen(mnt_fsname)) != 0) {
                    struct stat statbuf;
                    int ret;

                    ALOGD("block device path is %s", mnt_fsname);

                    ret = lstat(mnt_fsname, &statbuf);
                    if (ret < 0) {
                        ALOGE("stat error on %s(%s)", mnt_fsname, strerror(errno));
                        return -1;
                    }

                    if ((statbuf.st_mode & S_IFMT) == S_IFLNK) {
                        char buf[256];
                        ssize_t len;

                        ALOGD("%s is a symlink", mnt_fsname);
                        len = readlink(mnt_fsname, buf, 256);
                        if (len < 0) {
                            ALOGE("readlink error on %s(%s)", mnt_fsname, strerror(errno));
                            return -1;
                        }

                        buf[len] = '\0';
                        strcpy(block, buf);
                    } else {
                        strcpy(block, mnt_fsname);
                    }
                    fclose(fp);
                    return 0;
                }
            }
        }
        fclose(fp);
    }
    return -1;
}

int get_file_value(char *path, unsigned int *value) {
    FILE *fp = fopen(path, "r");
    if (fp) {
        char line[512];
        int ret;
        if (fgets(line, sizeof(line), fp)) {
            ret = sscanf(line, "%u", value);
            if (ret > 0) {
                fclose(fp);
                return 0;
            }
        }
        fclose(fp);
    }
    return -1;
}

int cksum_generate(char *path, unsigned int *value) {

    unsigned int crc = 0;
    unsigned char line[1024];
    static int first_run = true;

    if (first_run == true) {
        crc_init(crc_table);
        first_run = false;
    }

    android::base::unique_fd fp(open(path, O_RDONLY));

    if (fp == -1) {
        ALOGE("fail to open %s, errno %d", path, errno);
        return -1;
    }

    for (;;) {
        int len, i;
        len = read(fp, line, sizeof(line));
        if (len < 0) {
            ALOGE("fail to read %s, errno %d", path, errno);
            return -1;
        }

        if (len < 1)
            break;

        for (i = 0; i < len; i++)
            crc = cksum(crc, line[i]);
    }

    *value = crc;
    return 0;
}

int create_dir_if_needed(const char* pathname, mode_t mode) {
    struct stat st;

    int rc = stat(pathname, &st);
    if(rc != 0) {
        if (errno == ENOENT) {
            return mkdir(pathname, mode);
        } else {
            return rc;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        LOG(ERROR) << pathname << " is not a dir.";
        return -1;
    }

    mode_t actual_mode = st.st_mode & ALLPERMS;
    if (actual_mode != mode) {
        LOG(WARNING) << pathname << "perm " << actual_mode << " expected " << mode;
        chmod(pathname, mode);
    }

    return 0;
}

static int _delete_dir_contents(DIR* d,
        std::function<bool(const char*, bool)> exclusion_predicate = nullptr) {
    int result = 0;
    struct dirent *de;

    int dfd = dirfd(d);
    if (dfd < 0) return -1;

    while ((de = readdir(d))) {
        const char* name = de->d_name;

        if (exclusion_predicate && exclusion_predicate(name, (de->d_type == DT_DIR))) {
            continue;
        }

        if (de->d_type == DT_DIR) {
            int subfd;
            DIR* subdir;

            /* always skip "." and ".." */
            if (name[0] == '.') {
                if (name[1] == 0) continue;
                if ((name[1] == '.') && (name[2] == 0)) continue;
            }

            subfd = openat(dfd, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (subfd < 0) {
                ALOGE("Couldn't openat %s: %s\n", name, strerror(errno));
                result = -1;
                continue;
            }
            subdir = fdopendir(subfd);
            if (subdir == nullptr) {
                ALOGE("Couldn't fdopendir %s: %s\n", name, strerror(errno));
                close(subfd);
                result = -1;
                continue;
            }
            if (_delete_dir_contents(subdir, exclusion_predicate)) {
                result = -1;
            }
            closedir(subdir);
            if (unlinkat(dfd, name, AT_REMOVEDIR) < 0) {
                ALOGE("Couldn't unlinkat %s: %s\n", name, strerror(errno));
                result = -1;
            }
        } else {
            if (unlinkat(dfd, name, 0) < 0) {
                ALOGE("Couldn't unlinkat %s: %s\n", name, strerror(errno));
                result = -1;
            }
        }
    }

    return result;
}

int delete_dir_contents(const char* pathname, bool delete_dir,
        std::function<bool(const char*, bool)> exclusion_predicate) {
    int res = 0;
    DIR* dir = opendir(pathname);

    if (dir == nullptr) {
        if (errno == ENOENT) return 0;
        ALOGE("Couldn't opendir %s: %s\n", pathname, strerror(errno));
        return -errno;
    }

    res = _delete_dir_contents(dir, exclusion_predicate);

    closedir(dir);
    if (delete_dir) {
        if (rmdir(pathname)) {
            ALOGE("Couldn't rmdir %s: %s\n", pathname, strerror(errno));
            res = -1;
        }
    }
    return res;
}

int delete_dir_contents_and_dir(const char* pathname) {
    return delete_dir_contents(pathname, true, nullptr);
}

int visit_dir_contents(const char* pathname, bool recursive,
        std::function<void(const char* /*dname*/, bool /*is_dir*/)> visitor) {

    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(pathname), closedir);
    if (dir.get() == nullptr) {
        ALOGE("Couldn't opendir %s: %s\n", pathname, strerror(errno));
        return -errno;
    }

    struct dirent *de;
    while ((de = readdir(dir.get()))) {
        const char* name = de->d_name;

        /* always skip "." and ".." */
        if (name[0] == '.') {
            if (name[1] == 0) continue;
            if ((name[1] == '.') && (name[2] == 0)) continue;
        }

        if (visitor) {
            visitor(name, (de->d_type == DT_DIR));
        }

        if (de->d_type == DT_DIR && recursive) {
            // To do: visit the subdir
        }
    }

    return 0;
}

int copy_file(const char *fromPath, const char *toPath) {
    android::base::unique_fd fromFd(open(fromPath, O_RDONLY));
    if (fromFd == -1) {
        LOG(ERROR) << "Failed to open copy from " << fromPath << "  reason: " << strerror(errno);
        return -1;
    }
    android::base::unique_fd toFd(open(toPath, O_CREAT | O_WRONLY, 0664));
    if (toFd == -1) {
        LOG(ERROR) << "Failed to open copy to " << toPath << "  reason: " << strerror(errno);
        return -1;
    }

    struct stat sstat = {};
    if (stat(fromPath, &sstat) == -1) {
        LOG(ERROR) << "Failed to stat file: " << fromPath << "  reason: " << strerror(errno);
        return -1;
    }

    off_t length = sstat.st_size;
    int ret = 0;

    off_t offset = 0;
    while (offset < length) {
        ssize_t transfer_length = std::min(length - offset, (off_t) FILE_COPY_SIZE);
        ret = sendfile(toFd, fromFd, &offset, transfer_length);
        if (ret != transfer_length) {
            ret = -1;
            LOG(ERROR) << "Copying failed from:" << fromPath << " to " << toPath  << "  reason: " << strerror(errno);
            break;
        }
    }
    chown(toPath, getuid(), AID_SYSTEM);
    return ret == -1 ? -1 : 0;
}
