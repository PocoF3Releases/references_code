/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cutils/properties.h>
#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/xattr.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include <android-base/file.h>

#include "sysdeps.h"
#include "adb_io.h"
#include "fuse_adb_provider.h"
#include "mi_file_sync_service.h"
#include "miutil/mi_serial.h"
#include "mi_service.h"
#include "mi_miniz.c"

#define  TRACE_TAG  TRACE_SERVICES
#define MTDOOPS_LOG_FILE "/cache/recovery/mtdoops.md"

char region[PROPERTY_VALUE_MAX] = { 0 };
char language[PROPERTY_VALUE_MAX] = { 0 };
std::string mtpinstall_command;

static void write_string(int fd, const char* str) {
    WriteFdExactly(fd, str);
}

void read_from_father(int fd) {
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), from_father) != NULL) {
        char* command = strtok(buffer, " \n");
        if (command == NULL) {
            continue;
        } else {
            char str[128] = {0};
            sprintf(str, "%s\n", command);
            write_string(fd, str);
            adb_close(fd);
            break;
        }
    }
}

static void
load_locale_from_cache() {
    FILE* fp = fopen(LOCALE_FILE, "r");
    if (fp != NULL) {
        fscanf(fp, "%[^_]_%[^\n]", language, region);
        language[strlen(language)] ='\n';
        region[strlen(region)] ='\n';
        fflush(fp);
        if (ferror(fp)) printf("Error in %s\n(%s)\n", LOCALE_FILE, strerror(errno));
        fclose(fp);
        if (chmod(LOCALE_FILE, 0644) < 0) {
            printf("chmod error: %s\n", strerror(errno));
        }
    }
    if (!language[0] || !region[0]) {
        char info[PROPERTY_VALUE_MAX] = { 0 };
        property_get(LOCALE_KEY, info, "unknown");
        if (sscanf(info, "%[^-]-%[^\n]", language, region) != 2) {
            printf("can not get ro.product.locale.\n");
            return;
        }
        language[strlen(language)] ='\n';
        region[strlen(region)] ='\n';
    }

}

static void get_update_info(int fd, const std::string& args, const char *key)
{
    if (!args.empty()) printf("get_update_info arguments: %s\n", args.c_str());
    char info[PROPERTY_VALUE_MAX] = { 0 };
    property_get(key, info, "unknown");
    info[strlen(info)] ='\n';
    write_string(fd, info);
    adb_close(fd);
}

static void get_rom_zone(int fd, const std::string& args) {
    get_update_info(fd, args, ROM_ZONE_KEY);
}

static void get_minidump(int fd, const std::string& args) {
    compress_minidump();

    if (!access(PRESSED_MINI_DUMP, 0)) {
        write_string(fd, "success\n");
    } else {
        write_string(fd, "failure\n");
    }

    adb_close(fd);
}

static void get_mtdoopslog(int fd, const std::string& args) {

    const char *binary = "strings -n 3 /dev/mtd0 > /cache/recovery/mtdoops.md";
    char info[30] = { 0 };
    int status = 0;;

    status = system(binary);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        sprintf(info, "command fail:%d\n", WEXITSTATUS(status));
        write_string(fd, info);
        adb_close(fd);
        return;
     }

    if (!access(MTDOOPS_LOG_FILE, 0)) {
        write_string(fd, "success\n");
    } else {
        write_string(fd, "failure\n");
    }

    adb_close(fd);
}

static void get_version(int fd, const std::string& args) {
    get_update_info(fd, args, VERSION_KEY);
}

static void get_recovery_version(int fd, const std::string& args) {
    if (!args.empty()) printf("get_recovery_version arguments: %s\n", args.c_str());
    char info[16] = { 0 };
    sprintf(info, "%d", MIUI_RECOVERY_VERSION);
    info[strlen(info)] ='\n';
    write_string(fd, info);
    adb_close(fd);
}

static void get_codebase(int fd, const std::string& args) {
    get_update_info(fd, args, CODEBASE_KEY);
}

static void get_device(int fd, const std::string& args) {
    char info[PROPERTY_VALUE_MAX] = { 0 };
    property_get(MOD_DEVICE_KEY, info, "unknown");
     if (!strcmp(info, "unknown")) {
        get_update_info(fd, args, DEVICE_KEY);
        printf("can not get ro.product.mod_device.\n");
    }
    get_update_info(fd, args, MOD_DEVICE_KEY);
}

static void get_language(int fd, const std::string& args) {
    if (!args.empty()) printf("get_language arguments: %s\n", args.c_str());
    if (!language[0]) {
        load_locale_from_cache();
    }
    write_string(fd, language[0] ? language : "unknown\n");
    adb_close(fd);
}

static void get_region(int fd, const std::string& args) {
    if (!args.empty()) printf("get_region arguments: %s\n", args.c_str());
    if (!region[0]) {
        load_locale_from_cache();
    }
    write_string(fd, region[0] ? region : "unknown\n");
    adb_close(fd);
}

static void get_carrier(int fd, const std::string& args) {
    get_update_info(fd, args, CARRIER_KEY);
}

static void get_virtual_ab_enabled(int fd, const std::string& args) {
    get_update_info(fd, args, VIRTUAL_AB_ENABLED_KEY);
}

static void get_serial(int fd, const std::string& args) {
    if (!args.empty()) printf("get_serial arguments: %s\n", args.c_str());
    char info[PROPERTY_VALUE_MAX] = { 0 };
    char product_device[PROPERTY_VALUE_MAX] = { 0 };
    property_get("ro.product.device", product_device, "ooops");
    if (!strncmp(product_device, "omega", 5) || !strncmp(product_device, "nikel", 5)) {
        if (get_longcheer_sn(info) < 0)
            printf("Error: Get longcheer's serial number.\n");
    } else {
        if (get_miphone_sn(info) < 0)
            printf("Error: Get miphone's serial number.\n");
    }
    info[strlen(info)] ='\n';
    write_string(fd, info);
    adb_close(fd);
}

static bool is_development_version(void) {
    regex_t regex;
    int errorcode = 0;
    char errmsg[1024] = { 0 };
    size_t errmsglen = 0;
    const char* REGULAR_EXPRESSION_FOR_DEVELOPMENT = "^[[:digit:]]+.[[:digit:]]+.[[:digit:]]+(-internal)?"; // the pattern to judge if incremental_version is legal
    char incremental_version[PROPERTY_VALUE_MAX] = { 0 };

    property_get(VERSION_KEY, incremental_version, "unknown");

    if (incremental_version[0]) {
        if ((errorcode = regcomp(&regex, REGULAR_EXPRESSION_FOR_DEVELOPMENT, REG_EXTENDED)) == 0) {
            if ((errorcode = regexec(&regex, incremental_version, 0, NULL, 0)) == 0) {
                printf("%s:[%s] matches filter pattern %s\n", VERSION_KEY, incremental_version, REGULAR_EXPRESSION_FOR_DEVELOPMENT);
                regfree(&regex);
                return true;
            }
        }
        errmsglen = regerror(errorcode, &regex, errmsg, sizeof(errmsg));
        errmsglen = errmsglen < sizeof(errmsg) ? errmsglen : sizeof(errmsg) - 1;
        errmsg[errmsglen] = '\0';
        printf("filter pattern match failed: %s\n", errmsg);
        regfree(&regex);
    }

    return false;
}

static bool is_stable_version(void) {
    char build_type[PROPERTY_VALUE_MAX] = { 0 };
    property_get(TYPE_KEY, build_type, "unknown");
    bool isuesrtype = !strncmp("user", build_type, 4);
    bool isstable = isuesrtype && !is_development_version();
    return isstable;
}

static void get_branch(int fd, const std::string& args) {
    if (!args.empty()) printf("get_branch arguments: %s\n", args.c_str());
    if (is_stable_version()) {
        write_string(fd, "F\n");
    } else {
        write_string(fd, "X\n");
    }

    adb_close(fd);
}

static void format_data(int fd, const std::string& args) {
    if (!args.empty()) printf("format_data arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "format_data\n");
    read_from_father(fd);
}

static void format_cache(int fd, const std::string& args) {
    if (!args.empty()) printf("format_cache arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "format_cache\n");
    read_from_father(fd);
}

static void format_storage(int fd, const std::string& args) {
    if (!args.empty()) printf("format_storage arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "format_storage\n");
    read_from_father(fd);
}

static void format_data_and_storage(int fd, const std::string& args) {
    if (!args.empty()) printf("format_data_and_storage arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "format_data_and_storage\n");
    read_from_father(fd);
}

static void wipe_data_and_storage(int fd, const std::string& args) {
    if (!args.empty()) printf("wipe_data_and_storage arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "wipe_data_and_storage\n");
    read_from_father(fd);
}

static void reboot(int fd, const std::string& args) {
    if (!args.empty()) printf("reboot arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "reboot\n");
    read_from_father(fd);
    sleep(1);
    exit(0);
}

static void rebootbootloader(int fd, const std::string& args) {
    if (!args.empty()) printf("rebootbootloader arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "rebootbootloader\n");
    read_from_father(fd);
    sleep(1);
    exit(0);
}

static void rebootfastboot(int fd, const std::string& args) {
    if (!args.empty()) printf("rebootfastboot arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "rebootfastboot\n");
    read_from_father(fd);
    sleep(1);
    exit(0);
}

static void rebootrecovery(int fd, const std::string& args) {
    if (!args.empty()) printf("rebootrecovery arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "rebootrecovery\n");
    read_from_father(fd);
    sleep(1);
    exit(0);
}

static void shutdown(int fd, const std::string& args) {
    if (!args.empty()) printf("shutdown arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "shutdown\n");
    read_from_father(fd);
    sleep(1);
    exit(0);
}

static void wipe_efs(int fd, const std::string& args) {
    if (!args.empty()) printf("wipe_efs arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "wipe_efs\n");
    read_from_father(fd);
}

void sideload_verification(const char* saveptr) {
    char str[512] = { 0 };
    sprintf(str, "install_package:%s\n", saveptr);
    fprintf(g_cmd_pipe, "%s", str);
}

static void enable_mtp(int fd, const std::string& args) {
    if (!args.empty()) printf("enable_mtp arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "enablemtp\n");
    read_from_father(fd);
}

static void disable_mtp(int fd, const std::string& args) {
    if (!args.empty()) printf("disable_mtp arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "disablemtp\n");
    read_from_father(fd);
}

static void is_mtp_enabled(int fd, const std::string& args) {
    if (!args.empty()) printf("is_mtp_enabled arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "isenable\n");
    read_from_father(fd);
}

static void mtp_install(int fd, const std::string& args) {
    if (!args.empty()) printf("mtp_install arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "%s\n", mtpinstall_command.c_str());
    read_from_father(fd);
}

static void format_frp(int fd, const std::string& args) {
    if (!args.empty()) printf("format_frp arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "%s\n", args.c_str());
    read_from_father(fd);
}

static void getmitoken(int fd, const std::string& args) {
    if (!args.empty()) printf("getmitoken arguments: %s\n", args.c_str());
    fprintf(g_cmd_pipe, "%s\n", "getmitoken\n");
    read_from_father(fd);
}

// MIUI ADD：START
// add set_trap mode to logcat
static void settrap(int fd, const std::string& args) {
    if (!args.empty()) printf("settrap arguments: %s\n", args.c_str());
    std::string arg(args.substr(strlen("set_tarp:")));
    if (strlen(arg.c_str()) == 0) {
        if (!android::base::WriteStringToFile("true 60", LAST_TRAP_FILE)) {
            write_string(fd, "Failed to read set_trap to /cache/recovery/last_trap\n");
            exit(0);
        }
    }
    else if (strlen(arg.c_str()) >= 1) {
        int isNum = atoi(arg.c_str());
        if (isNum == 0) {
            write_string(fd, "bad set_trap number arguments failed\n");
            return;
        }

        if (!android::base::WriteStringToFile("true " + arg, LAST_TRAP_FILE)) {
            write_string(fd, "Failed to read set_trap to /cache/recovery/last_trap\n");
            exit(0);
        }
    }

    if (chmod(LAST_TRAP_FILE, 0644) < 0) { // ADD file permissions 0644
        printf("chmod error: %s\n", strerror(errno));
    }
    write_string(fd, "success\n");
    fprintf(g_cmd_pipe, "reboot\n");
    read_from_father(fd);
    sleep(1);
    exit(0);
}
//END

int service_mi_fd(int (*create_thread)(void (*)(int,  const std::string&), const std::string&), const char *name) {
    int ret = -1;
    if (!strncmp(name, "getversion:", 11)) {
        ret = create_thread(get_version, "\0");
    } else if (!strncmp(name, "getcodebase:", 12)) {
        ret = create_thread(get_codebase, "\0");
    } else if (!strncmp(name, "getdevice:", 10)) {
        ret = create_thread(get_device, "\0");
    } else if (!strncmp(name, "getbranch:", 10)) {
        ret = create_thread(get_branch, "\0");
    } else if (!strncmp(name, "getlanguage:", 12)) {
        ret = create_thread(get_language, "\0");
    } else if (!strncmp(name, "getregion:", 10)) {
        ret = create_thread(get_region, "\0");
    } else if (!strncmp(name, "getcarrier:", 11)) {
        ret = create_thread(get_carrier, "\0");
    } else if (!strncmp(name, "getvab_enabled", 14)) {
        ret = create_thread(get_virtual_ab_enabled, "\0");
    } else if (!strncmp(name, "getsn:", 6)) {
        ret = create_thread(get_serial, "\0");
    } else if (!strncmp(name, "getromzone:", 11)) {
        ret = create_thread(get_rom_zone, "\0");
    } else if (!strncmp(name, "getminidump", 11)) {
        ret = create_thread(get_minidump, "\0");
    } else if (!strncmp(name, "getmtdoopslog", 13)) {
        ret = create_thread(get_mtdoopslog, "\0");
    } else if (!strncmp(name, "format-data:", 12)) {
        ret = create_thread(format_data, "\0");
    } else if (!strncmp(name, "format-cache:", 13)) {
        ret = create_thread(format_cache, "\0");
    } else if (!strncmp(name, "format-storage:", 15)) {
        ret = create_thread(format_storage, "\0");
    } else if (!strncmp(name, "format-data-storage:", 20)) {
        ret = create_thread(format_data_and_storage, "\0");
    } else if (!strncmp(name, "wipe-data-storage:", 18)) {
        ret = create_thread(wipe_data_and_storage, "\0");
    } else if (!strncmp(name, "reboot:bootloader", 18)) {
        ret = create_thread(rebootbootloader, "\0");
    } else if (!strncmp(name, "reboot:fastboot", 16)) {
        ret = create_thread(rebootfastboot, "\0");
    } else if (!strncmp(name, "reboot:recovery", 15)) {
        ret = create_thread(rebootrecovery, "\0");
    } else if (!strncmp(name, "reboot:", 7) ) {
        ret = create_thread(reboot, "\0");
    } else if (!strncmp(name, "shutdown:", 9)) {
        ret = create_thread(shutdown, "\0");
    } else if (!strncmp(name, "wipe-efs:", 11)) {
        ret = create_thread(wipe_efs, "\0");
    } else if (!strncmp(name, "getrecoveryversion:", 19)) {
        ret = create_thread(get_recovery_version, "\0");
    } else if (!strncmp(name, "sync:", 5)) {
        ret = create_thread(file_sync_service, "\0");
    } else if (!strncmp(name, "enablemtp:", 10)) {
        ret = create_thread(enable_mtp, "\0");
    } else if (!strncmp(name, "disablemtp:", 11)) {
        ret = create_thread(disable_mtp, "\0");
    } else if (!strncmp(name, "mtpinstall:", 11)) {
        mtpinstall_command = name;
        ret = create_thread(mtp_install, mtpinstall_command);
    } else if (!strncmp(name, "isenable:", 9)) {
        ret = create_thread(is_mtp_enabled, "\0");
    } else if (!strncmp(name, "format-frp:", 11)) {
        ret = create_thread(format_frp, name);
    } else if (!strncmp(name, "getmitoken:", 11)) {
        ret = create_thread(getmitoken, "\0");
    } else if (!strncmp(name, "set_trap:", 9)) {
        ret = create_thread(settrap, name);
    }
    return ret;
}
