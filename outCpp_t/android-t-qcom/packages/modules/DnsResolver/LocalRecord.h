#ifndef __NETD_LOCAL_RECORD__
#define __NETD_LOCAL_RECORD__

#include <string>
#include <vector>

#include "utils/RWLock.h"

class LocalRecord {
  public:
    static void make();
    static LocalRecord* get();
    void addAddrInfo(const pid_t pid, const uid_t uid, const char* host, struct addrinfo* hints);
    void addAddrInfo(const pid_t pid, const uid_t uid, const char* msg);
    void dump(int fd);

  private:
    LocalRecord();

    std::string getAddrInfoLocked(size_t index);
    std::string inetSockaddrToString(const sockaddr* sa, const socklen_t sa_len);

    size_t mLastIndex;
    std::vector<std::string> mAddrInfos;
    static LocalRecord* sInstance;
    mutable android::RWLock mRWLock;

    static const size_t MAX_REC_SIZE;
};
#endif
