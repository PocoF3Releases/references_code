/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#define LOG_TAG "mqsasd"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include <regex.h>
#include <regex>
#include <vector>
#include <inttypes.h>
#include <openssl/aes.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/klog.h>
#include <sys/prctl.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>
#include <utils/String8.h>
#include "utils/Log.h"
#include <utils/SystemClock.h>
#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/unique_fd.h>
#include <android-base/strings.h>
#include <android-base/stringprintf.h>
#include <android-base/properties.h>

#include "dump.h"
#include "CmdTool.h"
#include "utils.h"
#include "MisysUtil.h"
#include "BootMonitorThread.h"
#include "TombstoneMonitorThread.h"

using namespace android;
using android::base::GetProperty;
using android::base::GetIntProperty;
using android::base::GetBoolProperty;
using android::base::StringPrintf;
using android::base::WriteFully;
using android::base::ReadFully;
using android::base::unique_fd;

#define SPEC_TAG "CaptureLogUtils"

#define LOGFILE_DIR "/data/mqsas"
#define PERSIST_DIR "/mnt/vendor/persist/stability"
#define UIDERRORS "/data/system/uiderrors.txt"
#define OFFLINE_LOGFILE_DIR "/data/system/miuiofflinedb/"

#define PROPERTIES_MTBF_TEST  "persist.mtbf.test"
#define PROPERTIES_OMNI_TEST  "persist.omni.test"
#define PROPERTIES_REBOOT_COREDUMP "persist.reboot.coredump"
#define PROPERTIES_MIUI_OFFLINEDB "persist.sys.miuiofflinedb"
#define PROPERTIES_MI_DEVLOPMENT "ro.mi.development"
#define PROPERTIES_MI_INTERNAL_TRIAL_USER "persist.sys.internal_trial_user"

#define MAX_DIR_FILE_NUM 15
#define MAX_LOG_FILES 5
#define DEFAULT_RUN_COMMAND_TIMEOUT 60

#define LOGV(...)    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...)    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...)    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...)    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...)    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static const int64_t NANOS_PER_SEC = 1000000000;
static const char* SUPPORT_ACTION = "cat,logcat,dumpsys,ps,top,df";
static const char* VENDOR_FILES = "/data/vendor/diag/last_kmsg,/data/vendor/diag/last_tzlog";
static const char* RESTARTLOG_DIR = "/data/miuilog/stability/reboot/";

static const std::string ANR_DIR = "/data/anr/";

int CaptureLogUtil::RUN_CMD_DEFAULT_TIMEOUT = 60;

static uint64_t nanotime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * NANOS_PER_SEC + ts.tv_nsec;
}

static bool ismtbftest() {
    return GetBoolProperty(PROPERTIES_MTBF_TEST, false);
}

static bool isomnitest() {
    return GetBoolProperty(PROPERTIES_OMNI_TEST, false);
}

static bool isUnrelease() {
    return GetBoolProperty(PROPERTIES_MI_DEVLOPMENT, false) ||  GetBoolProperty(PROPERTIES_MI_INTERNAL_TRIAL_USER, false);
}

static bool enablerebootcoredump() {
    return GetBoolProperty(PROPERTIES_REBOOT_COREDUMP, false);
}

static bool isNum(const char *str) {
    for (size_t i =0; i< strlen(str);i++)
    {
        size_t tmp = (size_t)str[i];
        if (tmp >=48 && tmp <=57) {
            continue;
        }
        else {
            return false;
        }
    }
    return true;
}

static std::vector<android::CaptureLogUtil::DumpData> GetDumpFds(const std::string& dir_path) {
    // 1. open /data/anr file dir
    LOGW("==================Start Get data/anr log for BootMonitor======================\n");
    std::unique_ptr<DIR, decltype(&closedir)> dump_dir(opendir(dir_path.c_str()), closedir);
    std::string build_fingerprint = GetProperty("ro.build.fingerprint", "");

    if (dump_dir == nullptr) {
        ALOGE("Unable to open directory %s: %s\n", dir_path.c_str(), strerror(errno));
        return std::vector<android::CaptureLogUtil::DumpData>();
    }
    std::vector<android::CaptureLogUtil::DumpData> dump_data;
    struct dirent* entry = nullptr;
    // 2. Readdir reads out all the files in the directory in sequence
    while ((entry = readdir(dump_dir.get()))) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        const std::string base_name(entry->d_name);

        const std::string abs_path = dir_path + base_name;
        chmod(abs_path.c_str(), (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP));
        android::base::unique_fd fd(
            TEMP_FAILURE_RETRY(open(abs_path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)));
        if (fd == -1) {
            ALOGE("Unable to open dump file %s: %s\n", abs_path.c_str(), strerror(errno));
            continue;
        }

        struct stat st = {};
        if (fstat(fd, &st) == -1) {
            ALOGE("Unable to stat dump file %s: %s\n", abs_path.c_str(), strerror(errno));
            continue;
        }

        // 3. Match build findeprint in anr file
        char buff[1024] = {0};
        int read_num = read(fd, buff, 1024);
        printf("====read: %d  ===========\n", read_num);
        std::regex build_printf_pattern(".*?Build fingerprint: '(.*?)'.*?");
        std::smatch results;
        std::string str = buff;
        if (regex_search(str, results, build_printf_pattern)) {
            if (results[1] == build_fingerprint) {
                LOGW("=============match success!!!=============\n");
                dump_data.emplace_back(android::CaptureLogUtil::DumpData{base_name, std::move(fd), st.st_mtime});
            }
            else {
                LOGW("read file for %d byte, build.fingerprint prop mismatch\n", read_num);
            }
        }
        else {
            LOGW("Faild to match %s\n", build_fingerprint.c_str());
        }
    }

    if (!dump_data.empty()) {
        std::sort(dump_data.begin(), dump_data.end(),
            [](const auto& a, const auto& b) { return a.mtime > b.mtime; });
    }

    return dump_data;
}

void CaptureLogUtil::get_files_from_persist(String8& files){
    DIR *dir;
    struct dirent *dirp;
    if ((dir = opendir(PERSIST_DIR)) == NULL) {
        mkdir(PERSIST_DIR,0755);
        LOGD("persist file does not exist ,create this dir.");
        return;
    }
    while ((dirp = readdir(dir)) != NULL) {
        char *dir_name = NULL;
        dir_name = dirp->d_name;
        if ((strcmp(".", dir_name) == 0) || (strcmp("..", dir_name) == 0)) {
            continue;
        }
        if (files.length() != 0) {
            files.append(",");
        }
        files.append(PERSIST_DIR);
        files.append("/");
        files.append(dir_name);
    }
    closedir(dir);
}

void CaptureLogUtil::read_file(const char* path, String8& content) {
    char buf[1024] = "";
    std::unique_ptr<FILE, int (*)(FILE*)> fptr(fopen(path, "r"), fclose);

    if (!fptr.get()) {
        LOGD("failed to open %s", path);
        return;
    }

    while ((fgets(buf, sizeof(buf)-1, fptr.get())) != NULL) {
       content.append(buf);
    }
}

int CaptureLogUtil::create_file_in_persist(const char* path) {
    DIR *dir;
    if ((dir = opendir(PERSIST_DIR)) == NULL) {
        mkdir(PERSIST_DIR,0755);
    }

    int fd = TEMP_FAILURE_RETRY(open(path, O_CREAT|O_WRONLY|O_NONBLOCK, 0666));
    if (fd < 0){
        LOGD("failed to open %s: %s", path, strerror(errno));
        return -1;
    }

    closedir(dir);
    close(fd);
    return 0;
}

void CaptureLogUtil::write_to_persist_file(const char* path, const char *buff){
    int fd = TEMP_FAILURE_RETRY(open(path, O_CREAT|O_WRONLY|O_NONBLOCK, 0666));
    if (fd < 0) {
        LOGD("failed to open %s: %s", path, strerror(errno));
            return ;
        }

    int ret = TEMP_FAILURE_RETRY(write(fd, buff, strlen(buff)));
    if (ret <= 0) {
        LOGD("read %s: %s", path, strerror(errno));
    }
    close(fd);
}

bool CaptureLogUtil::waitpid_with_timeout(pid_t pid, int timeout_seconds, int* status) {
    sigset_t child_mask, old_mask;
    sigemptyset(&child_mask);
    sigaddset(&child_mask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &child_mask, &old_mask) == -1) {
        LOGE("*** sigprocmask failed: %s\n", strerror(errno));
        return false;
    }

    struct timespec ts;
    ts.tv_sec = timeout_seconds;
    ts.tv_nsec = 0;
    int ret = TEMP_FAILURE_RETRY(sigtimedwait(&child_mask, NULL, &ts));
    int saved_errno = errno;
    // Set the signals back the way they were.
    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1) {
        LOGE("*** sigprocmask failed: %s\n", strerror(errno));
        if (ret == 0) {
            return false;
        }
    }
    if (ret == -1) {
        errno = saved_errno;
        if (errno == EAGAIN) {
            errno = ETIMEDOUT;
        } else {
            LOGE("*** sigtimedwait failed: %s\n", strerror(errno));
        }
        return false;
    }

    pid_t child_pid = waitpid(pid, status, WNOHANG);
    if (child_pid != pid) {
        if (child_pid != -1) {
            LOGE("*** Waiting for pid %d, got pid %d instead\n", pid, child_pid);
        } else {
            LOGE("*** waitpid failed: %s\n", strerror(errno));
        }
        return false;
    }
    return true;
}

static std::string build_zip_file_path(const char* filepath, const char* zip_dir) {
    std::string zip_file("");
    struct stat fileStat;

    if (zip_dir && access(zip_dir, F_OK) == 0
            && stat(zip_dir, &fileStat) == 0
            && S_ISDIR(fileStat.st_mode)
            && access(zip_dir, W_OK) == 0
            && zip_dir[strlen(zip_dir)-1] == '/') {
        // strip the dir of 'filepath'
        const char* ch1 = filepath;
        const char* ch2 = ch1;
        while (ch1 && ch1[0]) {
            if (ch1[0] == '/'){
                ch2 = ch1;
                ch2++;
            }
            ch1++;
        }

        /* if zip_dir = "/a/b/" &&  filepath = "c/d"
         * then zip_file = "a/b/d.zip"
         * */
        zip_file.append(zip_dir);
        zip_file.append(ch2);
    } else {
        zip_file.append(filepath);
    }

    zip_file.append(".zip");

    return zip_file;
}

int CaptureLogUtil::do_compress(const char* filepath,
                                const char* zip_dir,
                                Vector<String16> *include_files,
                                int timeout,
                                bool isRecordInLocal) {
    int result = 0;
    std::string zip_file = build_zip_file_path(filepath, zip_dir);

    std::vector<std::string> zip_cmd = {
        "zip_utils", "-o", "-6", "-j", zip_file, filepath};

    bool includeDbg = false;
    bool includeCore = false;
    Vector<String8> todelete_files;
    if (include_files!= nullptr) {
        for(size_t i = 0; i < include_files->size(); i++) {
            String8 include8(include_files->itemAt(i));

            if (include8.find("/data/miuilog/stability/nativecrash") >= 0) {
                todelete_files.push_back(include8);
                includeCore = true;
                // 2G core, compress time is more than 60s, so the time out is set to 5min
                timeout += 4*60;
            }

            includeDbg |= (include8.find("gbg") >= 0);

            if (0 == access(include8.c_str(), F_OK)) {
                int index = 0;
                include_zip_files(include8.c_str(), zip_cmd, &index);
            }
        }
    }

    result = run_command(zip_cmd, filepath, timeout);

    if (includeDbg) {
        std::string mtk_param = StringPrintf("%s,%s.mtk", zip_file.c_str(), zip_file.c_str());
        CmdTool cmdtool;
        cmdtool.run_command_lite("mv", mtk_param.c_str(), timeout);
        zip_file.append(".mtk");
    }

    if (strstr(filepath, "_ne_") != NULL && includeCore) {
        bool skipRemoveCore = ismtbftest() || enablerebootcoredump();
        for (size_t i = 0; i < todelete_files.size(); i++) {
            if (skipRemoveCore) {
                LOGE("skip delete file(%s) \n", todelete_files[i].c_str());
            } else {
                // Prevent synchronized tombstone file from being deleted
                std::string tombstoneMonitorDir = android::base::GetProperty(TOMBSTONE_COPY_TO_DIR_PROP, TOMBSTONE_COPY_TO_DIR);
                if (todelete_files[i].find(tombstoneMonitorDir.c_str()) != -1) {
                    LOGW("skip delete file(%s) \n", todelete_files[i].c_str());
                    continue;
                }
                LOGE("delete file(%s) \n", todelete_files[i].c_str());
                if ((access(todelete_files[i], F_OK)) != -1) {
                    if (remove(todelete_files[i].c_str()) == -1) {
                        LOGW("failed to remove file(%s) \n", todelete_files[i].c_str());
                    }
                }
            }
        }
    }

    chown(zip_file.c_str(), AID_ROOT, AID_SYSTEM);
    chmod(zip_file.c_str(), S_IRWXU | S_IRWXG | S_IROTH);

    if(GetBoolProperty(PROPERTIES_MIUI_OFFLINEDB, false)) {
        copy_offlinefile(zip_file);
    }

    if (isRecordInLocal && strstr(filepath, "/data/mqsas/") != NULL) {
        LOGE("start copy log which ruleid is not -1");
        mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
        int rc = create_dir_if_needed(RESTARTLOG_DIR, mode);
        if (rc == 0) {
            chown(RESTARTLOG_DIR, AID_SYSTEM, AID_SYSTEM);
            chmod(RESTARTLOG_DIR, mode);
            std::string param = "-p," + zip_file + "," + RESTARTLOG_DIR;
            CmdTool cmdtool;
            cmdtool.run_command_lite("cp", param.c_str(), DEFAULT_RUN_COMMAND_TIMEOUT);
            LOGE("copy %s", filepath);
        }
    }

    return result;
}

void CaptureLogUtil::include_zip_files(const char *input, std::vector<std::string> &zip_cmd, int *index) {
    if (*index > MAX_DIR_FILE_NUM) {
        return;
    }

    struct stat st;
    if (stat(input, &st) == 0 && S_ISDIR(st.st_mode)) {
        std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(input), closedir);
        if (dir.get() == nullptr) {
            LOGE("Error in open dir %s", input);
            return;
        }

        struct dirent *dp;
        while ((dp = readdir(dir.get())) != NULL) {
            if ((strcmp(".", dp->d_name) == 0) || (strcmp("..", dp->d_name) == 0)) {
                continue;
            }

            std::string child = StringPrintf("%s/%s", input, dp->d_name);
            include_zip_files(child.c_str(), zip_cmd, index);
        }
    } else {
        zip_cmd.emplace_back(std::string(input));
        if (strcmp(input, UIDERRORS) != 0) {
            chmod(input, S_IRWXU | S_IRWXG | S_IRWXO);
        }
        (*index)++;
    }
}

void CaptureLogUtil::encript_zip(const char* basename){
    std::unique_ptr<FILE, int (*)(FILE*)> input(fopen(basename, "rb"), fclose);
    if (!input.get()) {
        LOGE("Can not open zip file.\n");
        return;
    }

    std::string miz_name = StringPrintf("%s.miz", basename);
    std::unique_ptr<FILE, int (*)(FILE*)> output(fopen(miz_name.c_str(), "wb"), fclose);
    if (!output.get()) {
        LOGE("Fail to open to write z.\n");
        return;
    }

    unsigned char iv_enc[AES_BLOCK_SIZE] = SPEC_TAG;
    for (int i = 0; i != AES_BLOCK_SIZE; ++i) {
        if (iv_enc[i] == 'i') {
            iv_enc[i] = 'u';
        } else if (iv_enc[i] == 'u') {
            iv_enc[i] = 'e';
        } else if (iv_enc[i] == 'e') {
            iv_enc[i] = 'o';
        }
    }
    iv_enc[AES_BLOCK_SIZE - 1] = 'X';

    unsigned char aes_key[AES_BLOCK_SIZE];
    int aes_key_index = 0;

    int bn_len = strlen(basename);
    for (int i = bn_len - 1; i && aes_key_index != AES_BLOCK_SIZE; --i) {
        if ('0' <= basename[i] && basename[i] <= '9') {
            aes_key[aes_key_index] = basename[i];
            ++ aes_key_index;
        }
    }

    for (int i = 0; iv_enc[i] != '\0' && aes_key_index != AES_BLOCK_SIZE; ++i) {
        aes_key[aes_key_index] = iv_enc[i];
        ++ aes_key_index;
    }

    fwrite(SPEC_TAG, AES_BLOCK_SIZE, 1, output.get());

    AES_KEY enc_key;
    AES_set_encrypt_key(aes_key, AES_BLOCK_SIZE * 8, &enc_key);
    unsigned char switch_buf;
    unsigned char buffer[AES_BLOCK_SIZE];
    unsigned char encbuffer[AES_BLOCK_SIZE];
    size_t bytes_read = 0;
    while (!!(bytes_read = fread(buffer, 1, AES_BLOCK_SIZE, input.get()))) {
        AES_cbc_encrypt(buffer, encbuffer, bytes_read, &enc_key, iv_enc, AES_ENCRYPT);
        for (int i = 0; i != AES_BLOCK_SIZE / 2; ++ i) {
            switch_buf = encbuffer[i];
            encbuffer[i] = encbuffer[AES_BLOCK_SIZE - 1 - i];
            encbuffer[AES_BLOCK_SIZE - 1 - i] = switch_buf;
        }
        fwrite(encbuffer, AES_BLOCK_SIZE, 1, output.get());
    }

    chown(miz_name.c_str(), AID_SYSTEM, AID_SYSTEM);
    chmod(miz_name.c_str(), S_IRWXU | S_IRWXG);
}

void CaptureLogUtil::redirect_output(const char* path) {
    int flags = O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC;
    int fd = TEMP_FAILURE_RETRY(open(path, flags, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
    LOGD("redirect_output: %s\n", path);
    if (fd < 0) {
        fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
        exit(1);
    }

    TEMP_FAILURE_RETRY(dup2(fd, STDOUT_FILENO));
    TEMP_FAILURE_RETRY(dup2(fd, STDERR_FILENO));

    close(fd);
}

int CaptureLogUtil::run_command(const std::vector<std::string>& full_cmds,
                                const char* log_path, int timeout) {
    uint64_t start = nanotime();
    std::string cmd_str;
    std::vector<const char*> args;
    int size = full_cmds.size() + 1; // null terminated
    args.resize(size);

    for (size_t i = 0; i < full_cmds.size(); i++) {
        args[i] = full_cmds[i].data();
        cmd_str += args[i];
        if (i != full_cmds.size() - 1) {
            cmd_str += " ";
        }
    }
    args[size-1] = nullptr;
    const char* action = args[0];
    const char* command = cmd_str.c_str();
    LOGE("run_command: %s\n", command);

    pid_t pid = fork();
    if (pid < 0) {
        LOGE("*** fork: %s\n",strerror(errno));
        return pid;
    }

    // handle child case
    if (pid == 0) {
        redirect_output(log_path);

        // Don't buffer stdout and stderr
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);

        // make sure the child dies when parent dies
        prctl(PR_SET_PDEATHSIG, SIGKILL);

        // just ignore SIGPIPE, will go down with parent's
        struct sigaction sigact;
        memset(&sigact, 0, sizeof(sigact));
        sigact.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sigact, NULL);


        printf("------ (%s) in %ds ------\n", command, timeout);
        fflush(stdout);
        execvp(action, (char**)args.data());
        printf("*** exec(%s): %s\n", action, strerror(errno));
        fflush(stdout);
        _exit(-1);
    }

    // handle parent case
    int status;
    if (ismtbftest() || isomnitest()) {
        RUN_CMD_DEFAULT_TIMEOUT = 90;
    }
    int timeout_seconds = (timeout <= 0) ? RUN_CMD_DEFAULT_TIMEOUT : timeout;
    bool ret = waitpid_with_timeout(pid, timeout_seconds, &status);
    uint64_t elapsed = nanotime() - start;

    FILE *fp_raw = fopen(log_path, "a+");
    if (!fp_raw) {
        LOGE("Open log file failed. err: %s. log path: %s\n", strerror(errno), log_path);
        return -1;
    }

    std::unique_ptr<FILE, int (*)(FILE*)> fptr(fp_raw, fclose);
    fflush(fptr.get());
    fflush(stdout);

    if (!ret) {
        if (errno == ETIMEDOUT) {
            fprintf(fptr.get(),"*** %s: Timed out after %.3fs (killing pid %d)\n", action,
                   (float) elapsed / NANOS_PER_SEC, pid);
            LOGE("*** %s: Timed out after %.3fs (killing pid %d)\n", action,
                   (float) elapsed / NANOS_PER_SEC, pid);
        } else {
            fprintf(fptr.get(),"*** %s: Error after %.4fs (killing pid %d)\n", action,
                   (float) elapsed / NANOS_PER_SEC, pid);
            LOGE("*** %s: Error after %.4fs (killing pid %d)\n", action,
                   (float) elapsed / NANOS_PER_SEC, pid);
        }
        kill(pid, SIGTERM);
        if (!waitpid_with_timeout(pid, 5, NULL)) {
            kill(pid, SIGKILL);
            if (!waitpid_with_timeout(pid, 5, NULL)) {
                fprintf(fptr.get(),"*** %s: Cannot kill %d even with SIGKILL.\n", action, pid);
            }
        }
        return -1;
    }

    if (WIFSIGNALED(status)) {
        fprintf(fptr.get(),"*** %s: Killed by signal %d\n", action, WTERMSIG(status));
    } else if (WIFEXITED(status) && WEXITSTATUS(status) > 0) {
        fprintf(fptr.get(),"*** %s: Exit code %d, %d\n", action, WEXITSTATUS(status), pid);
    }
    fprintf(fptr.get(),"[%s: %.3fs elapsed]\n\n", action, (float)elapsed / NANOS_PER_SEC);
    fflush(fptr.get());

    return 0;
}

size_t num_props = 0;
static char* props[2000];

static void print_prop(const char *key, const char *name, void *user) {
    (void) user;
    if (num_props < sizeof(props) / sizeof(props[0])) {
        char buf[PROPERTY_KEY_MAX + PROPERTY_VALUE_MAX + 10];
        snprintf(buf, sizeof(buf), "[%s]: [%s]\n", key, name);
        props[num_props++] = strdup(buf);
    }
}

static int compare_prop(const void *a, const void *b) {
    return strcmp(*(char * const *) a, *(char * const *) b);
}

/* prints all the system properties */
void CaptureLogUtil::print_properties(const char* output) {
    size_t i;
    num_props = 0;
    property_list(print_prop, NULL);
    qsort(&props, num_props, sizeof(props[0]), compare_prop);

    const char* imei_pattern = "[0-9]{15,17}";
    regex_t regex;
    regcomp(&regex, imei_pattern, REG_EXTENDED);

    std::unique_ptr<FILE, int (*)(FILE*)> fptr(fopen(output, "a+"), fclose);
    fprintf(fptr.get(),"------ SYSTEM PROPERTIES ------\n");
    for (i = 0; i < num_props; ++i) {
        regmatch_t pmatch;
        int reti = regexec( &regex, props[i], 0, &pmatch, 0 );
        if(reti != REG_NOMATCH && pmatch.rm_so != -1) {
            continue;
        }
        fputs(props[i], fptr.get());
        free(props[i]);
    }
    regfree(&regex);
}

void CaptureLogUtil::do_dmesg(const char* output) {
    std::unique_ptr<FILE, int (*)(FILE*)> fptr(fopen(output, "a+"), fclose);
    if (!fptr.get()) {
        LOG(ERROR) << "failed to open log file " << output << " :" << strerror(errno);
        return;
    }

    int size = klogctl(KLOG_SIZE_BUFFER, NULL, 0);
    if (size <= 0) {
        fprintf(fptr.get(),"Unexpected klogctl return value: %d\n\n", size);
        return;
    }

    char *buf = (char *) malloc(size + 1);
    if (buf == NULL) {
        fprintf(fptr.get(),"memory allocation failed\n\n");
        return;
    }

    int retval = klogctl(KLOG_READ_ALL, buf, size);
    if (retval < 0) {
        fprintf(fptr.get(),"klogctl failure\n\n");
        free(buf);
        return;
    }

    buf[retval] = '\0';
    fprintf(fptr.get(),"------ KERNEL LOG (dmesg) ------\n");
    fprintf(fptr.get(),"%s\n\n", buf);

    free(buf);
}

static char* replace_special_char_withX(String16& headline16) {
    String8 headline8(headline16);
    if (headline8.size() == 0) { headline8.setTo("null"); }

    char* title = new char[headline8.size() + 1];
    strlcpy(title, headline8.string(), headline8.size() + 1);

    char* tmp = title;
    static const char SpecialChars[] = "\\/:*\?'\"<>|&(){}%";
    const char* special_chars = SpecialChars;
    while(tmp && *tmp){
        special_chars = SpecialChars;
        while (*special_chars != '\0') {
            if (*tmp == *special_chars) {
                *tmp ='X';
            }
            special_chars++;
        }
        tmp++;
    }

   return title;
}

static void wait_for_coredump(Vector<String16>& includeFs) {
    static int maxcount = 15;
    static int maxwaitcount = 3;

    for (size_t i = 0; i < includeFs.size(); i++) {
        String8 include8(includeFs[i]);

        if (include8.find("/data/miuilog/stability/nativecrash") < 0) {
            continue;
        }

        const char* filename = include8.c_str();
        if (GetBoolProperty("persist.sys.device_config_gki", false)
                || GetBoolProperty("persist.sys.enable_miui_coredump", false)) {
            // gki 2.0机型或者mtk 机型core_pattern 设置为绝对路径，core文件生成时间会滞后，需要等待3S
            int waitcount = 0;
            for(; waitcount < maxwaitcount; waitcount++) {
                struct stat st;
                if (stat(filename, &st) == 0) {
                    LOGE("coredump(%s) file has created\n", filename);
                    break;
                }
                LOGE("wait coredump(%s) created %d times \n", filename, waitcount+1);
                sleep(1);
            }
        }

        int64_t filesize = 0;
        int count = 0;
        for(; count < maxcount; count++) {
            struct stat st;
            if (stat(filename, &st) != 0) {
                LOGE("coredump(%s) file stat fail: %s\n", filename, strerror(errno));
                break;
            }

            LOGE("wait coredump(%s) %d times, filesize=%" PRId64 "\n", filename, count+1, st.st_size);

            // 等待1s 后如果文件大小没有发生变化,我们认为已经dump完成
            // To do: 不一定准确!
            if ((st.st_size > 0) && (st.st_size == filesize)) {
                LOGE("dump coredump(%s) completed, filesize:%" PRId64 "\n", filename, filesize);
                break;
            } else {
                filesize = st.st_size;
            }

            sleep(1);
        }

        if (count >= maxcount) {
            LOGE("dump coredump(%s) tiemout, filesize=%" PRId64 "\n", filename, filesize);
        }
    }
}

void CaptureLogUtil::dump_all(String16& type,
                            String16& headline,
                            Vector<String16>& actions,
                            Vector<String16>& params,
                            bool offline,
                            int32_t id,
                            bool upload,
                            String16& where,
                            Vector<String16>& includedFiles,
                            bool isRecordInLocal) {

    LOGD("isRecordInLocal is %s\n", isRecordInLocal ? "true" : "false");

    if (actions.empty() || params.empty() || (actions.size() != params.size())) {
        LOGE("Error in dump: actions or params exception.");
        return;
    }

    String8 type8(type);
    String8 where8(where);

    char* title = replace_special_char_withX(headline);

    char *path = find_candidate_log_file(type8.string(),
                                        offline,
                                        id,
                                        title,
                                        upload,
                                        where8.string(),
                                        isRecordInLocal);
    if(!path) {
        delete[] title;
        return;
    }
    LOGI("dump_all path = %s",path);

    // date && title
    {
        std::unique_ptr<FILE, int (*)(FILE*)> filePtr(fopen(path, "a+"), fclose);
        if (!filePtr.get()) {
            LOG(ERROR) << "failed to open log file " << path << " :" << strerror(errno);
            free(path);
            delete[] title;
            return;
        }

        fprintf(filePtr.get(), "========================================================\n");
        time_t now = time(NULL);
        char date[64] = "";
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", localtime(&now));

        fprintf(filePtr.get(), "== date: %s\n", date);
        fprintf(filePtr.get(), "== headline: %s\n", title);
        fprintf(filePtr.get(), "== debuggable: %s\n", GetProperty("ro.debuggable", "0").c_str());
        fprintf(filePtr.get(), "========================================================\n");
    }
    delete[] title;

    // uptime
    run_command({"uptime"}, path, 1);

    // wait for native coredump complete
    if (strcmp(type8.string(), "ne") == 0) {
        wait_for_coredump(includedFiles);
    }

    int compress_timeout = DEFAULT_RUN_COMMAND_TIMEOUT*2;
    for(size_t i = 0; i < actions.size(); i++){
        String8 action8(actions[i]);
        String8 param8(params[i]);

        LOGE("dump begin path=%s (%s %s) id=%d", path, action8.c_str(), param8.c_str(), id);

        if (action8 == "do_dumpheap") {
            compress_timeout = 10*60;
        } else if (action8 == "do_dmesg") {
            do_dmesg(path);
        } else if (action8 == "cat" && param8.find(VENDOR_FILES) >= 0) {
            // kernel log & tz log
            std::unique_ptr<FILE, int (*)(FILE*)> fptr(fopen(path, "a+"), fclose);
            MisysUtil misys;
            misys.cat_vendor_file(param8.c_str(), fptr.get());
        } else if (strstr(SUPPORT_ACTION, action8.c_str())) {
            std::vector<std::string> full_cmds;
            full_cmds.emplace_back(action8.c_str());

            // split param with ', '
            const char* param = param8.c_str();
            if (param != NULL) {
                char* each = strtok((char*)param, ", ");
                while (each) {
                    full_cmds.emplace_back(each);
                    each = strtok(NULL, ", ");
                }
            }
            if (ismtbftest() || isomnitest()) {
                run_command(full_cmds, path);
            } else {
                run_command(full_cmds, path, 5);
            }
        }
    }

    if (strcmp(type8.string(), "hangTemp") != 0) {
        do_compress(path, where8.string(), &includedFiles, compress_timeout, isRecordInLocal);

        if (remove(path) != 0) {
            LOGE("failed to remove %s, error: %s", path, strerror(errno));
        }
    }

    // free after strdup
    free(path);
}

char* CaptureLogUtil::find_candidate_log_file(const char* type,
                                        bool offline,
                                        int id,
                                        const char* headline,
                                        bool upload,
                                        const char* where,
                                        bool isRecordInLocal) {
    if (strcmp("RescuePartyLog", type) != 0 && strcmp("BootFailure", type) != 0) {
        // where: /cache/mqsas/
        if (isRecordInLocal && id <= -1 && !isUnrelease() && !(ismtbftest() || isomnitest())) {
            where = RESTARTLOG_DIR;
            create_dir_if_needed(where, S_IRWXU | S_IRWXG | S_IRWXO);
        } else {
            where = "/data/mqsas/";
        }
    }

    if (strlen(where) + strlen(type) >= 100) {
        LOGE("The length of where or type is too long and the type is %s\n",type);
        return NULL;
    }

    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
    std::string logDir;
    std::string xline_dir;
    if (strcmp(where, RESTARTLOG_DIR) == 0) {
        xline_dir = where;
    } else {
        logDir = StringPrintf("%s/%s", where, type);
        int rc = create_dir_if_needed(logDir.c_str(), mode);
        if (rc != 0) {
            LOG(ERROR) << "failed to create log dir "
                       << logDir.c_str() << " :" << strerror(errno);
            return NULL;
        }
        chown(logDir.c_str(), AID_SYSTEM, AID_SYSTEM);
        chmod(logDir.c_str(), mode);

        xline_dir = StringPrintf("%s/%s", logDir.c_str(), offline ? "offline" : "online");
        rc = create_dir_if_needed(xline_dir.c_str(), mode);
        if (rc != 0) {
            LOG(ERROR) << "failed to create dir " << xline_dir.c_str() << " :" << strerror(errno);
            return NULL;
        }
        chown(xline_dir.c_str(), AID_SYSTEM, AID_SYSTEM);
        chmod(xline_dir.c_str(), mode);
    }

    LOGD("logfile_template:xline_dir=%s", xline_dir.c_str());

    if (offline) {
        return get_offline_file(type, headline, xline_dir.c_str());
    } else {
        return get_online_file(type, id, headline, upload, xline_dir.c_str());
    }
}

char* CaptureLogUtil::get_online_file(const char* type,
                                    int id,
                                    const char* headline,
                                    bool upload,
                                    const char* online_dir) {
    //-1为体验版抓取log默认的ruleId
    //-2为bugreport的ruleId
    std::string rule_id = (id >= -2 && upload) ? StringPrintf("%d", id) : StringPrintf("null");

    time_t t = time(NULL);
    int ii = time(&t);
    srand((unsigned)t);
    std::string device = GetProperty("ro.product.device", "unknown");
    std::string miui_version = GetProperty("ro.build.version.incremental", "unknown");

    std::string path = StringPrintf("%s/%s_%s_%s_%s_%s_%d_%d",
                                    online_dir,
                                    rule_id.c_str(),
                                    type,
                                    headline,
                                    device.c_str(),
                                    miui_version.c_str(),
                                    ii,
                                    rand());

    int _flags = O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW | O_CLOEXEC;
    unique_fd fd(open(path.c_str(), _flags, 0600));
    if (fd < 0) {
        LOGE("failed to open tombstone file '%s': %s\n", path.c_str(), strerror(errno));
        return NULL;
    }
    fchown(fd, AID_SYSTEM, AID_SYSTEM);
    fchmod(fd, 0666);

    return strdup(path.c_str());
}

char* CaptureLogUtil::get_offline_file(const char* type, const char* headline, const char* offline_dir) {
    int best = -1;
    struct stat best_sb;

    for (int i = 0; i < MAX_LOG_FILES; i++) {
        std::string path = StringPrintf("%s/%s_%d", offline_dir, type, i);
        std::string zip_path = path + ".zip";

        struct stat sb;
        int rc = stat(zip_path.c_str(), &sb);
        if (rc != 0 && errno == ENOENT) {
            best = i;
            break;
        }

        if (best < 0 || sb.st_mtime < best_sb.st_mtime) {
            best = i;
            best_sb.st_mtime = sb.st_mtime;
        }
    }

    if (best < 0) {
        LOGE("Failed to find a valid tombstone, default to using tombstone 0.\n");
        best = 0;
    }

    std::string path = StringPrintf("%s/%s_%s_%d", offline_dir, type, headline, best);

    int _flags = O_CREAT | O_EXCL | O_TRUNC | O_WRONLY | O_NOFOLLOW | O_CLOEXEC;
    unique_fd fd(open(path.c_str(), _flags, 0600));
    if (fd < 0) {
        LOGE("failed to open tombstone file '%s': %s\n", path.c_str(), strerror(errno));
        return NULL;
    }
    fchown(fd, AID_SYSTEM, AID_SYSTEM);
    fchmod(fd, 0666);

    return strdup(path.c_str());
}

void CaptureLogUtil::dump_lite_bugreport(const char* zip_dir, const char* bugreport_path) {
    // create the bugreport zip file
    unique_fd bugreport_fd(TEMP_FAILURE_RETRY(
                open(bugreport_path, O_CREAT|O_WRONLY|O_TRUNC|O_CLOEXEC|O_NOFOLLOW, 0664)));
    chown(bugreport_path, AID_ROOT, AID_SYSTEM);
    chmod(bugreport_path, 0664);
    bugreport_fd.reset();

    // Start miuibmdump, which runs dumpstate binary with "-b -f"
    // which captures lite bugreport and save to ${sys.miui.bm_zip}
    property_set("sys.miui.bm_zip", bugreport_path);
    property_set("ctl.start", "miuibmdump");

    // Connect to dumpstate and wait for completed
    unique_fd socket_fd;
    for (int i = 0; i< 20; i++) {
        socket_fd.reset(socket_local_client("dumpstate",
                    ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM));
        if (socket_fd >= 0) {
            break;
        }
        usleep(500000); // 500ms
    }

    if (socket_fd.get() < 0) {
        LOG(ERROR) << "Failed to connect dumpstate socket: " << strerror(errno);
        return;
    } else {
        if (fcntl(socket_fd.get(), F_SETFD, FD_CLOEXEC) == -1) {
            LOG(ERROR) << "Failed to set the flag of opened file descriptor: " << strerror(errno);
            return;
        }

        struct timeval tv = {
            .tv_sec = 30,
            .tv_usec = 0,
        };
        if (setsockopt(socket_fd.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0){
            char buf[256] = {'0'};
            if (!ReadFully(socket_fd.get(), buf, sizeof(buf)-1)) {
                LOG(ERROR) << "Read buf failed: " << strerror(errno);
            }
            LOG(DEBUG) << "read response from dumpstate: " << buf;

            // Tear down
            property_set("ctl.stop", "miuibmdump");
            property_set("sys.miui.bm_zip", "");
        }
    }
}

void CaptureLogUtil::dump_for_actions(const char *zip_dir, const char *log_path,
        std::vector<DumpAction> &actions) {
    {
        std::unique_ptr<FILE, int (*)(FILE*)> fptr(fopen(log_path, "a+"), fclose);
        if (!fptr.get()) {
            LOG(ERROR) << "failed to open " << log_path << " :" << strerror(errno);
            return;
        }
        fprintf(fptr.get(), "========================================================\n");
        time_t now = time(NULL);
        char date[64] = "";
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", localtime(&now));

        fprintf(fptr.get(), "=== Date: %s ===\n", date);
        fprintf(fptr.get(), "========================================================\n");
    }

    size_t size = actions.size();
    for (size_t i = 0; i < size; i++) {
        const std::string title = actions[i].title;
        if (!title.empty()) {
            std::unique_ptr<FILE, int (*)(FILE*)> fp(fopen(log_path, "a+"), fclose);
            if (fp.get()) {
                fprintf(fp.get(), "%s\n", title.c_str());
            }
        }
        run_command(actions[i].cmd, log_path, actions[i].timeout);
    }
    int anr_file_num = 0;
    std::string service_watchdog_timestamp = GetProperty("sys.service.watchdog.timestamp", "");
    LOGW("get watchdog timestamp: %s\n", service_watchdog_timestamp.c_str());
    Vector<String16> include_files;
    include_files.add(String16(UIDERRORS));

    // 1. Check whether the sys.service.watchdog.timestamp attribute exists
    if (service_watchdog_timestamp != "") {
        if (isNum(service_watchdog_timestamp.c_str())) {
            if (atoi(service_watchdog_timestamp.c_str()) < 3 * 60 * 60 * 1000) {
                auto anr_data_ = GetDumpFds(ANR_DIR);
                for (auto it = anr_data_.begin(); it != anr_data_.end(); ++it) {
                    std::string &name = it->name;
                    include_files.add(String16((ANR_DIR + name).c_str()));
                    anr_file_num++;
                    if(anr_file_num > 9) break;
                }
            }
        }
    } else {
        LOGW("failed to get watchdog timestamp prop ");
    }

    if (do_compress(log_path, zip_dir, &include_files, /*int timeout*/ 5) == 0) {
        remove(log_path);
    } else {
        std::string toPath = StringPrintf("%s/uiderrors.txt", zip_dir);
        copy_file(UIDERRORS, toPath.c_str());
    }
}

bool CaptureLogUtil::create_file_write(const char* path, const char *buff) {
    bool result = true;

    int fd = TEMP_FAILURE_RETRY(open(path, O_CREAT|O_WRONLY|O_NONBLOCK, 0666));
    if (fd < 0) {
        LOGD("failed to open %s: %s", path, strerror(errno));
        return false;
    }

    int ret = TEMP_FAILURE_RETRY(write(fd, buff, strlen(buff)));
    if (ret <= 0) {
        LOGD("read %s: %s", path, strerror(errno));
        result = false;
    }

    close(fd);
    return result;
}

void CaptureLogUtil::flash_debugpolicy(int32_t type) {
    static const char* flashcmds1[] = {
        "dd if=/data/mqsas/download/apdp.mbn of=/dev/block/bootdevice/by-name/apdp",
        "dd if=/data/mqsas/download/msadp.mbn of=/dev/block/bootdevice/by-name/msadp",
        "chmod 777 /data/mqsas/download/apdp.mbn",
        "chmod 777 /data/mqsas/download/msadp.mbn",
    };
    static const char* flashcmds2[] = {
        "dd if=/data/mqsas/download/dp_AP_signed.mbn of=/dev/block/bootdevice/by-name/apdp",
        "dd if=/data/mqsas/download/dp_MSA_signed.mbn of=/dev/block/bootdevice/by-name/msadp",
        "chmod 777 /data/mqsas/download/dp_AP_signed.mbn",
        "chmod 777 /data/mqsas/download/dp_MSA_signed.mbn",
    };
    int ret = system("chmod 777 /data/mqsas/download");
    LOGD("change dic %d", ret);

    size_t size = (type == 1) ? (sizeof(flashcmds1)/sizeof(flashcmds1[0]))
                              : (sizeof(flashcmds2)/sizeof(flashcmds2[0]));
    for (size_t i = 0; i < size; i++) {
        const char* cmd = (type == 1) ? flashcmds1[i] : flashcmds2[i];
        ret = system(cmd);
        LOGD("flash debugpolicy cmd[%zu] ret=%d\n", i, ret);
    }
}

void CaptureLogUtil::copy_offlinefile(const std::string file)
{
    std::string sourcefile_path = file;
    size_t index = sourcefile_path.find_last_of('/');
    std::string targetfile_name = sourcefile_path.substr(index);
    std::vector<std::string> file_info;
    std::stringstream out;
    int iPosEnd = 0;

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    out << std::put_time(std::localtime(&now), "%Y-%m-%d-%H-%M-%S");
    std::string date = out.str();
    mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

    while (true) {
        iPosEnd = targetfile_name.find('_');
        if (iPosEnd == -1) {
            file_info.push_back(targetfile_name);
            break;
        }
        file_info.push_back(targetfile_name.substr(0, iPosEnd));
        targetfile_name = targetfile_name.substr(iPosEnd + 1);
    }

    std::string targetfile_path = OFFLINE_LOGFILE_DIR
                                     + file_info[1] + "_"
                                     + date + "_"
                                     + file_info[5] + "_"
                                     + file_info[6] + "_"
                                     + file_info[7] + "_"
                                     + file_info[3] + ".zip";

    copy_file(sourcefile_path.c_str(), targetfile_path.c_str());
    chmod(targetfile_path.c_str(),mode);
}
