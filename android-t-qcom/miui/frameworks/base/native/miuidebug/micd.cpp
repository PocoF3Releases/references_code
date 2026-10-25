/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syscall.h>
#include <unistd.h>
#include <android-base/logging.h>
#include <log/log.h>
#include <sys/utsname.h>
#include <time.h>

#define BUF_SIZE 16384
#define KTHREADD 2
#define PUSH_BLOCK_ACK_NS 10000000L
int
main(int argc, char *argv[])
{
    ssize_t nread;
    ssize_t nwrite;
    char buf[BUF_SIZE];
    char *ptr;
    char name[256] = {0};
    int fd;
    static const struct timespec req = {0, PUSH_BLOCK_ACK_NS};

    if (getppid() != KTHREADD) {
        struct utsname u;
        if (uname(&u) == 0) {
            int a, b, c;
            sscanf(u.release,"%d%*c%d%*c%d", &a, &b, &c);
            LOG(DEBUG) << "Kernel version: " << a;

            if (a >= 4) {
                LOG(ERROR) << "No permission to run it";
                return -1;
            }
        } else {
            LOG(ERROR) << "Open kernel version file failed ";
            return -1;
        }
    }

    if (argc < 3)
        return -1;

    setgid(1000);
    sprintf(name, "/data/miuilog/stability/nativecrash/core/core-%s-%s", argv[1], argv[2]);

    for (int i = 0; i < 10; i++) {
        if (access(name, F_OK)) {
            //wait 10ms to create corefile
            LOG(INFO) << "corefile is not exist, wait 10ms";
            nanosleep(&req, NULL);
        } else {
            LOG(INFO) << "corefile: " << name;
            break;
        }
    }

    fd = open(name, O_RDWR, 0660);
    if (fd < 0) {
        LOG(ERROR) << "open failed "<< name << "  erro:" << strerror(errno);
        return -1;
    }

    while (nread = read(STDIN_FILENO, buf, BUF_SIZE))
    {
        if ((nread == -1) && (errno != EINTR)) {
            LOG(ERROR) << "read pipe failed: ";
            break;
        } else if (nread > 0) {
            ptr = buf;
            while (nwrite = write(fd, ptr, nread)) {
                if ((nwrite == -1) && (errno != EINTR)) break;
                else if (nwrite == nread) break;
                else if (nwrite > 0) {
                    ptr += nwrite;
                    nread -= nwrite;
                }
            }
            if (nwrite == -1) {
                LOG(ERROR) << "write failed: ";
                break;
            }
        }
    }

    close(fd);
    return 0;
}
