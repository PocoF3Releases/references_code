#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <errno.h>
#include <string.h>
#include <utils/Mutex.h>

#include "octvm.h"
#include "boost.h"

using namespace android;

static Mutex sMutex;
int devfd = -1;
int is_support = UNKNOWN;
int is_freq_limit = UNKNOWN;
int freq_limit_first = 0;

static inline void decrypt_content(char *content, int len)
{
    for (int i = 0; i < len; i++) {
        content[i] ^= DECRYPT_MAGIC_BASE;
    }
}

static int check_in_config(const char *package_name)
{
    FILE *fp;
    char line[BUF_SIZE];
    int ret;
    char *pFreqLimit = NULL;
    fp = fopen(CONFIG_PATH, "r");
    if (!fp) {
        fprintf(stderr, "fail to open file: %s", strerror(errno));
        return NOT_SUPPORT;
    }
    memset(line, 0, BUF_SIZE);
    while (fgets(line, BUF_SIZE-1, fp) != NULL) {
        int ll = 0;
        // Why? Because when content is encrytped, some character'll be encrypted to 0
        // Such 'u' xor 117 is 0. Then if use strlen to check string length,
        // the string'll be cut off. So use this to get string's length
        while ((ll < BUF_SIZE - 1) && line[ll] != '\n') {
            ll++;
        }
        line[ll] = '\0';
        decrypt_content(line, ll);
        if (strncmp(line, package_name, strlen(package_name)) == 0) {
            pFreqLimit = strchr(line, ':');
            if (pFreqLimit != NULL) {
                is_freq_limit = SUPPORT;
                fprintf(stderr, "is_freq_limit = %d", is_freq_limit);
            }
            fclose(fp);
            return SUPPORT;
        }
    }
    fclose(fp);
    return NOT_SUPPORT;
}

static int check_in_minor_window()
{
    FILE *fp;
    int minor_window_uid = 0;
    int uid = getuid();
    fp = fopen(PATH_MINOR_WINDOW, "r");
    if (!fp) {
        fprintf(stderr, "fail to open node: %s", strerror(errno));
        return NOT_SUPPORT;
    }
    if(fscanf(fp, "%d", &minor_window_uid))
    {
      if(minor_window_uid == uid)
      {
        fclose(fp);
        return SUPPORT;
      }    
    }
    fclose(fp);

    return NOT_SUPPORT;
}

static int check_packge_valid(void)
{
    if (is_support == SUPPORT || is_support == NOT_SUPPORT) {
        return is_support;
    }

    char proc_pid_path[BUF_SIZE];
    char package_name[BUF_SIZE];
    int pid = getpid();
    sprintf(proc_pid_path, "/proc/%d/cmdline", pid);

    FILE *fp = fopen(proc_pid_path, "r");
    if (!fp) {
        fprintf(stderr, "fail to open %s: %s", proc_pid_path, strerror(errno));
        return UNKNOWN;
    }
    if (!fgets(package_name, BUF_SIZE-1, fp)) {
        fprintf(stderr, "fail to read: %s", strerror(errno));
        fclose(fp);
        return UNKNOWN;
    }
    is_support = check_in_config(package_name);
    if(is_support == NOT_SUPPORT){
        is_support = check_in_minor_window();
    }
    fclose(fp);
    return is_support;
}

static int check_perf_ioctl_valid(void)
{
    if (devfd >= 0) {
        return 0;
    } else if (devfd == -1) {
        devfd = open(PATH_PERF_IOCTL, O_RDWR);
        // file not exits
        if (devfd < 0 && errno == ENOENT) {
            devfd = -2;
        }
        // file exist, but can't open
        if (devfd == -1) {
            fprintf(stderr, "Can't open device: %s", strerror(errno));
            return -1;
        }
    // file not exist
    } else if (devfd == -2) {
        fprintf(stderr, "Can't open dev: %s", strerror(errno));
        return -2;
    }
    return 0;
}

static int check_freq_limit_need(void)
{
    if (is_freq_limit == SUPPORT && !freq_limit_first) {
        freq_limit_first = 1;
        fprintf(stderr, "check_freq_limit_need, need =%d",  freq_limit_first);
        return SUPPORT;
    }
    return UNKNOWN;
}
extern "C"
int xgfNotifyQueue(__u32 a)
{
    Mutex::Autolock lock(sMutex);
    int size = -1;

    if (check_packge_valid() == SUPPORT) {
        if (check_perf_ioctl_valid() == 0) {
            ioctl(devfd, TRIGGER_GAME_BOOST, &size);
            if (check_freq_limit_need() == SUPPORT) {
                ioctl(devfd, TRIGGER_GAME_CEILING, &size);
            }
        }
    }
    return 0;
}

extern "C"
int notifyVsync(int type, int size)
{
    // Mutex::Autolock lock(sMutex);
    if (check_perf_ioctl_valid() == 0) {
        ioctl(devfd, TRIGGER_GAME_VSYNC, &size);
    }

    return 1;
}
