#define LOG_TAG "MIUINDBG"
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <paths.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <openssl/aes.h>
#include <string>
#include <vector>
#include <sockets.h>
#include <log/log.h>

#define MIUI_NATIVE_DEBUG_ROOT_DIR          "/data/miuilog/stability/nativecrash"

#define MIUI_NATIVE_FREE_SPACE_MIN          (unsigned long)0x40000000
#if defined(__LP64__)
#define MIUI_NATIVE_FREE_SPACE_MAX          (unsigned long)(0x180000000)  //6G
#else
#define MIUI_NATIVE_FREE_SPACE_MAX          (unsigned long)(0xC0000000)  //3G
#endif

#define RULE_DUMP_ENABLE            (unsigned char)1

/*none default full*/
#define RULE_CORE_TYPE_DEFAULT      (unsigned char)1
#define RULE_CORE_TYPE_FULL         (unsigned char)2

typedef struct rule_file {
    struct rule_file*   next;
    char*               p_str;
    unsigned char       c_type;
    unsigned char       c_action;
} RULE_FILE_T;

typedef struct rule_dump_info_t {
    unsigned int       core:2;
    unsigned int       jstk:2;
    unsigned int       mlog:1;
    unsigned int       slog:1;
    unsigned int       elog:1;
    unsigned int       rlog:1;
    unsigned int       klog:1;
    unsigned int       maps:1;
    unsigned int       tomb:1;
    unsigned int       ps:1;
    unsigned int       lsof:1;
    unsigned int       spec:1;
    RULE_FILE_T*       p_file;
} RULE_DI_T;

typedef struct rule_t {
    struct rule_t*      next;
    int                 i_id;
    char*               p_pn;
    char*               p_bt;
    RULE_DI_T           t_di;
} RULE_T;

static bool s_dump_enable = true;
static RULE_T m_rule = {0};
static unsigned long s_core_limit = MIUI_NATIVE_FREE_SPACE_MIN/4*3;

#define print_errno() { ALOGE("Line: %d, errno: %d\n", __LINE__, errno); }

struct rlimit64_t {
    __u64 rlim_cur;
    __u64 rlim_max;
};

static void core_set_limit(pid_t pid,unsigned long size)
{
    if (!pid || pid == getpid()) {
        struct rlimit rlim;
        rlim.rlim_cur = size;
        rlim.rlim_max = size;
        ALOGE("core_set_limit rlim size = %lu pid = %d \n", size, pid);
        int res = setrlimit(RLIMIT_CORE, &rlim);
        if (res) {
          ALOGE("core_set_limit rlim size = %lu pid = %d failed \n", size, pid);
          print_errno();
        }
    } else {
        struct rlimit64_t rlim64;
        rlim64.rlim_cur = size;
        rlim64.rlim_max = size;
        ALOGE("core_set_limit rlim64 size = %lu pid = %d \n", size, pid);
        syscall(__NR_prlimit64, pid, RLIMIT_CORE, &rlim64, NULL);
    }
}

static void calculate_rlimit()
{
    struct statfs stfs;

    if (statfs(MIUI_NATIVE_DEBUG_ROOT_DIR, &stfs) >= 0) {
        unsigned long long freeSize = (unsigned long long)(stfs.f_bfree*stfs.f_bsize);

        if (freeSize < MIUI_NATIVE_FREE_SPACE_MIN) {
            ALOGE("calculate_rlimit s_core_limit = %lu \n", s_core_limit);
            s_core_limit = 0;
            s_dump_enable = false;
        } else {
            if (freeSize > MIUI_NATIVE_FREE_SPACE_MAX)
                freeSize = MIUI_NATIVE_FREE_SPACE_MAX;

            s_core_limit = (freeSize/4*3)/0x10000*0x10000;
            ALOGE("calculate_rlimit s_core_limit = %lu \n", s_core_limit);
        }
    } else {
        ALOGE("/data/miuilog/stability/nativecrash is not exist\n");
        print_errno();
    }
}

static void dump_core(pid_t pid, pid_t tid, RULE_T* rule)
{
    ALOGE("dump_core pid = %d core_rule = %u \n", pid, rule->t_di.core);
    if (!rule->t_di.core) return;
    calculate_rlimit();
    core_set_limit(pid,s_core_limit);
}

static void dump_info(pid_t pid, pid_t tid, RULE_T* rule)
{
    if (!rule) return;
    dump_core(pid, tid, rule);
}

bool WriteFullyFd(int fd, const void* data, size_t byte_count) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
  size_t remaining = byte_count;
  while (remaining > 0) {
    ssize_t n = TEMP_FAILURE_RETRY(write(fd, p, remaining));
    if (n == -1) return false;
    p += n;
    remaining -= n;
  }
  return true;
}

static bool mqsas_manager_notify(pid_t pid,  pid_t tid) {
  int mfd = socket_local_client(/*"/data/system/ndebugsocket"*/ "mqsas_native_socket\0",
          /*ANDROID_SOCKET_NAMESPACE_FILESYSTEM*/ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
  if (mfd == -1) {
    ALOGE("unable to connect to mqsas native socket\n");
    return false;
  }

  struct timeval tv = {
    .tv_sec = 1,
    .tv_usec = 0,
  };
  if (setsockopt(mfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
    ALOGE("failed to set send timeout on mqsas native socket\n");
    close(mfd);
    return false;
  }
  tv.tv_sec = 5;  // 5 seconds on handshake read
  if (setsockopt(mfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
    ALOGE("failed to set receive timeout on mqsas native socket\n");
    close(mfd);
    return false;
  }
  // get tombstone path according to fd
  {
    char buf[64] = {'\0'};
    snprintf(buf,sizeof(buf), "%d,%d", pid, tid);
    if (!WriteFullyFd(mfd, buf, strlen(buf))) {
      ALOGE("crash_dump(miuindbg) client write pid tid failed\n");
      close(mfd);
      return false;
    }
  }
  // 3 sec timeout reading the ack; we're fine if the read fails.
  char ack[256];
  memset(&ack, 0, sizeof(ack));
  char *p = &ack[0];
  int sum = 0, len = 0;
  while ((len = read(mfd, p + sum, sizeof(ack) - sum - 1)) > 0)
  {
    sum += len;
    if (*(p+sum-1) == '}')
      break;
  }
  if ((sum > 0) && (*(p+sum-1) != '}'))
  {
    ALOGE("crash_dump(miuindbg) client read ack failed\n");
    close(mfd);
    return false;
  } else {
    ALOGE("crash_dump(miuindbg) client read ack success\n");
  }
  *(p+sum) = '\0';
  // parse the reading info to rules
  // {"core":"none","lsof":"","jstack":"none","toolversion":"6.7.1"}
  memset(&m_rule, 0, sizeof(m_rule));
  {
    char *tmp;
    if (tmp = strstr(p, "core")) {
      if (*(tmp+5) == ':' && *(tmp+6) == '"') {
        char *end = strchr(tmp+7, '"');
        if (end) {
          if (!strncmp("default", tmp+7, end - (tmp+8)))
            m_rule.t_di.core = RULE_CORE_TYPE_DEFAULT;
          else if (!strncmp("full", tmp+7, end - (tmp+8)))
            m_rule.t_di.core = RULE_CORE_TYPE_FULL;
        }
      }
    }
    ALOGE("core type: %d \n", m_rule.t_di.core);
  }
  close(mfd);
  return true;
}


extern "C" void miui_native_debug_process_O(pid_t pid, pid_t tid)
{
    ALOGE("miui_native_debug_process_O\n");

    if(mqsas_manager_notify(pid, tid))
        dump_info(pid, tid, &m_rule);
}
