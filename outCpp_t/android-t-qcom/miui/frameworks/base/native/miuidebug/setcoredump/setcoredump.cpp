#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <paths.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <dirent.h>

//#include "ndbglog.h"

static const char* gProgName = "setcoredump";
static bool kFullDump = false;

/*
 * Shows usage.
 */
static void usage(void) {
  fprintf(stderr, "\nUsage:\n");
  fprintf(stderr, "\t%s -h: show this help message.\n", gProgName);
  fprintf(stderr, "\t%s -f: enable full dump on this device.\n", gProgName);
  fprintf(stderr, "\t%s -n: enable normal dump on this device.\n", gProgName);
  fprintf(stderr, "\t%s -p [pid] : enable normal dump for one process [pid].\n", gProgName);
  fprintf(stderr, "\t%s -f -p [pid]: enable full dump for one process [pid].\n\n", gProgName);
  fprintf(stderr, "core file directory: /data/core/\n\n");
}

typedef void (for_each_pid_func)(int, const char *);

static void __for_each_pid(void (*helper)(int, const char *, void *), void *arg)
{
    DIR *d;
    struct dirent *de;

    if (!(d = opendir("/proc"))) {
        printf("Failed to open /proc (%s)\n", strerror(errno));
        return;
    }

    while ((de = readdir(d))) {
        int pid;
        int fd;
        char cmdpath[255];
        char cmdline[255];

        if (!(pid = atoi(de->d_name))) {
            continue;
        }

        sprintf(cmdpath,"/proc/%d/cmdline", pid);
        memset(cmdline, 0, sizeof(cmdline));
        if ((fd = TEMP_FAILURE_RETRY(open(cmdpath, O_RDONLY | O_CLOEXEC))) < 0) {
            strcpy(cmdline, "N/A");
        } else {
            read(fd, cmdline, sizeof(cmdline) - 1);
            close(fd);
        }
        helper(pid, cmdline, arg);
    }

    closedir(d);
}

static void for_each_pid_helper(int pid, const char *cmdline, void *arg)
{
    for_each_pid_func *func = (for_each_pid_func *) arg;
    func(pid, cmdline);
}

static void for_each_pid(for_each_pid_func func)
{
    __for_each_pid(for_each_pid_helper, (void *) func);
}

static void core_set_filter(pid_t pid, bool is_full)
{
    char path[PATH_MAX];


    snprintf(path,sizeof(path),"/proc/%d/coredump_filter",pid);

    int fd = open(path, O_WRONLY);

    if (fd > 0) {
        if (is_full) {
            write(fd, "39", 2);    /*0x27 MMF_DUMP_ANON_PRIVATE|MMF_DUMP_ANON_SHARED|MMF_DUMP_MAPPED_PRIVATE|MMF_DUMP_HUGETLB_PRIVATE*/
        } else {
            write(fd, "35", 2);    /*0x23 MMF_DUMP_ANON_PRIVATE|MMF_DUMP_ANON_SHARED|MMF_DUMP_HUGETLB_PRIVATE*/
        }
        close(fd);
    }
}

static void core_set_limit(pid_t pid,unsigned long size)
{
    if (!pid || pid == getpid()) {
        struct rlimit rlim;

        rlim.rlim_cur = size;
        rlim.rlim_max = size;
        setrlimit(RLIMIT_CORE, &rlim);
    } else {
        struct rlimit64 rlim64;

        rlim64.rlim_cur = size;
        rlim64.rlim_max = size;
        prlimit64(pid, RLIMIT_CORE, &rlim64, NULL);
    }
}

static void set_rlimit_and_coredump_filter(int pid, const char * proc_name) {
    if (pid == 1 || proc_name == NULL || strlen(proc_name) < 1) return;

//    fprintf(stderr, "hanli, %d, %s\n", pid, proc_name);

    core_set_filter(pid, kFullDump);
    core_set_limit(pid, RLIM_INFINITY);
}

static void do_cmd(char* cmd,bool is_sync)
{
//    MILOGE("do_cmd %s",cmd);

    pid_t pid = fork();
    if (!pid) {
        char *argp[4] = {(char*)"sh", (char*)"-c", NULL,NULL};

        argp[2] = (char *)cmd;
        int ret = execv(_PATH_BSHELL,argp);
//        MILOGE("ret=%d,err=%d",ret,errno);
        _exit(-1);
    } else if (is_sync) {
        struct stat sb;
        char proc[PATH_MAX];
        int us = 100000;

        snprintf(proc, sizeof(proc),"/proc/%d",pid);

        while (us < 2000000 && !stat(proc, &sb)) {
           usleep(us);
//           MILOGD("do_cmd cmd = %s sleep ns = %d", cmd, us);
           us = us*2;
        }
    }
}

static bool is_legal_pid(char* pid) {
    if (pid == NULL || strlen(pid) < 1) return false;

    for(size_t i = 0; i < strlen(pid); i++) {
        if (pid[i] > '9' || pid[i] < '0') return false;
    }

    return true;
}

static bool directory_exists(const char* name) {
  struct stat st;
  if (stat(name, &st) == 0) {
    return S_ISDIR(st.st_mode);
  } else {
    return false;
  }
}

int setcoredumpDriver(int argc, char** argv) {

    int full_dump  = -1;
    pid_t pid = -1;
    bool show_help_msg = false;

    while (1) {
        const int ic = getopt(argc, argv, "p:n::f::h::");
        if (ic < 0) {
            if (argv[optind] != NULL) {
                fprintf(stderr, "\nunrecognized option: %s\n", argv[optind]);
                return -2;
            }
            if (full_dump == -1 && pid == -1) {
                if (!show_help_msg)
                    fprintf(stderr, "\n%s: should have an option value as bellow: \n", gProgName);
                return -2;
            } else
                break;
        }

        switch(ic) {
            case 'f':
                if (full_dump != -1) {
                    fprintf(stderr, "[-n] option should not use with [-f] option.\n");
                    return -2;
                }
                if (optarg != NULL) {
                    fprintf(stderr, "[-f] option should not have a value: %s\n", optarg);
                    return -2;
                }
                full_dump = 1;
            break;
            case 'n':
                if (full_dump != -1) {
                    fprintf(stderr, "[-n] option should not use with [-f] option.\n");
                    return -2;
                }
                if (optarg != NULL) {
                    fprintf(stderr, "[-n] option should not have a value: %s\n", optarg);
                    return -2;
                }
                full_dump = 0;
            break;
            case 'p':
                if (!is_legal_pid(optarg)) {
                    fprintf(stderr, "[-p] option should have a legal process id value.\n");
                    return -2;
                }
                pid = atoi(optarg);
            break;
            case 'h':
                if (full_dump != -1 || pid != -1) {
                    fprintf(stderr, "[-h] option should not use with [-f -n -p] option at the same time.\n");
                    return -2;
                }
                if (optarg != NULL) {
                    fprintf(stderr, "[-h] option should not have a value: %s\n", optarg);
                    return -2;
                }
                show_help_msg = true;
            break;
            default:
                return -2;
            break;
        }
    }
    if (show_help_msg && (full_dump != -1 || pid != -1)) {
        fprintf(stderr, "[-h] option should not use with [-f -n -p] option at the same time.\n");
        return -2;
    }

    char cmd[PATH_MAX];

    if (pid != -1) {
        snprintf(cmd, sizeof(cmd), "/proc/%d/", pid);
        if (!directory_exists(cmd)) {
            fprintf(stderr, "process [%d] does not exist !", pid);
            return -1;
        }
    }

    snprintf(cmd, sizeof(cmd), "setprop persist.debug.trace 1");
    do_cmd(cmd, false);

    snprintf(cmd, sizeof(cmd), "setenforce 0");
    do_cmd(cmd, false);

    kFullDump = (full_dump == 1);

    if (pid == -1) {
        for_each_pid(set_rlimit_and_coredump_filter);
    } else {
        core_set_filter(pid, kFullDump);
        core_set_limit(pid, RLIM_INFINITY);
    }

    return 0;
}


int main(int argc, char** argv) {

    if (argc > 4) {
        usage();
        return -1;
    }

    int i = 0;
/*
    while (i < argc) {
        fprintf(stderr, "\n[%d]: %s\n", i, argv[i]);
        i++;
    }
*/
    i  = setcoredumpDriver(argc, argv);
    if (i < 0) {
        if (i == -1) {
            fprintf(stderr, "\nsetcoredump FAILED !\n");
        }
        usage();
    } else {
        fprintf(stderr, "\nsetcoredump [%s] SUCCESS.\n", kFullDump ? "FULL" : "NORMAL");
    }

    return 0;
}
