#include <stdio.h>
#include <string.h>
#include <time.h>

#include <cutils/log.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include "LocalRecord.h"

const size_t LocalRecord::MAX_REC_SIZE = 1000;
LocalRecord* LocalRecord::sInstance = NULL;

void LocalRecord::make() {
    get();
}

LocalRecord* LocalRecord::get() {
    if (!sInstance) {
        sInstance = new LocalRecord();
    }
    return sInstance;
}

LocalRecord::LocalRecord() : mLastIndex(0) {
    mAddrInfos.clear();
}

std::string LocalRecord::inetSockaddrToString(const sockaddr* sa, const socklen_t sa_len) {
    char host[NI_MAXHOST] = {0};
    if (getnameinfo(sa, sa_len, host, sizeof(host), nullptr, 0, NI_NUMERICHOST)) {
        return std::string();
    }
    return std::string(host);
}

void LocalRecord::addAddrInfo(const pid_t pid, const uid_t uid, const char* host,
                              struct addrinfo* hints) {
    if (host == NULL) {
        return;
    }

    std::string msg(host);
    if (!msg.empty()) {
        std::string::size_type pos = msg.rfind('.');
        if (pos == std::string::npos) {
            pos = msg.size() - 1;
        }
        msg.replace(1, pos + 1 - 1, "*.");
        msg.append("=");
    }

    for (struct addrinfo* ai = hints; ai != nullptr; ai = ai->ai_next) {
        if (ai->ai_addr) {
            std::string ssa = inetSockaddrToString(ai->ai_addr, ai->ai_addrlen);
            if (!ssa.empty()) {
                std::string::size_type dot = ssa.find('.');
                if (dot == std::string::npos) {
                    dot = ssa.size() - 1;
                }
                msg.append(ssa, 0, dot + 1).append("*").append(",");
            }
        }
    }

    if (!hints) {
        msg.append(" <get addr info failed>");
    }

    return addAddrInfo(pid, uid, msg.c_str());
}

void LocalRecord::addAddrInfo(const pid_t pid, const uid_t uid, const char* msg) {
    std::string line;

    char now[64];
    time_t tm = time(NULL);
    strftime(now, sizeof(now), "%H:%M:%S", localtime(&tm));
    line.append(now).append(" ");

    line.append(std::to_string(pid)).append(" ");
    line.append(std::to_string(uid)).append(" ");
    if (msg) {
        line.append(" ").append(msg);
    }

    android::RWLock::AutoWLock _l(mRWLock);
    if (mAddrInfos.size() < MAX_REC_SIZE) {
        mAddrInfos.push_back(line);
    } else {
        mAddrInfos[mLastIndex] = line;
        mLastIndex += 1;
        if (mLastIndex >= MAX_REC_SIZE) {
            mLastIndex = 0;
        }
    }
}

std::string LocalRecord::getAddrInfoLocked(size_t index) {
    size_t pos = mLastIndex + index;
    if (pos > MAX_REC_SIZE) {
        pos -= MAX_REC_SIZE;
    }
    if (pos < mAddrInfos.size()) {
        return mAddrInfos[pos];
    }
    return std::string();
}

void LocalRecord::dump(int fd) {
    dprintf(fd, "  Dns Resolve Records\n");

    {
        android::RWLock::AutoRLock _l(mRWLock);
        dprintf(fd, "  total records=%lu\n", (unsigned long)mAddrInfos.size());
        for (size_t i = 0; i < mAddrInfos.size(); i++) {
            std::string line("rec[");
            line.append(std::to_string(i));
            line.append("]: ");
            line.append(getAddrInfoLocked(i));
            dprintf(fd, "    %s\n", line.c_str());
        }
    }

    dprintf(fd, "\n");
}
