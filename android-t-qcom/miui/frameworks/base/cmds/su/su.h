/*
** Copyright 2010, Adam Shanks (@ChainsDD)
** Copyright 2008, Zinx Verituse (@zinxv)
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef SU_h
#define SU_h 1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LBE-Sec"

#ifndef AID_SHELL
#define AID_SHELL (get_shell_uid())
#endif

#ifndef AID_ROOT
#define AID_ROOT  0
#endif

#ifndef AID_SYSTEM
#define AID_SYSTEM (get_system_uid())
#endif

#ifndef AID_RADIO
#define AID_RADIO (get_radio_uid())
#endif

#ifndef JAVA_PACKAGE_NAME
#define JAVA_PACKAGE_NAME "com.lbe.security.miui"
#endif

// If --rename-manifest-package is used in AAPT, this
// must be changed to correspond to the new APK package name
// See the two Android.mk files for more details.
#ifndef REQUESTOR
#define REQUESTOR JAVA_PACKAGE_NAME
#endif
// This is used if wrapping the fragment classes and activities
// with classes in another package. CM requirement.
#ifndef REQUESTOR_PREFIX
#define REQUESTOR_PREFIX JAVA_PACKAGE_NAME
#endif
#define REQUESTOR_DATA_PATH "/data/data/"
#define REQUESTOR_FILES_PATH REQUESTOR_DATA_PATH REQUESTOR "/files"
#define REQUESTOR_USER_PATH "/data/user/"
#define REQUESTOR_DAEMON_PATH REQUESTOR ".daemon"
#define REQUESTOR_CONTENT_PROVIDER REQUESTOR ".su"

#define DEFAULT_SHELL "/system/bin/sh"

#define xstr(a) str(a)
#define str(a) #a

#ifndef VERSION_CODE
#define VERSION_CODE 15
#endif
#define VERSION xstr(VERSION_CODE) " " REQUESTOR

#define PROTO_VERSION 1

struct su_initiator {
    pid_t pid;
    unsigned uid;
    unsigned user;
    char name[64];
    char bin[PATH_MAX];
    char args[4096];
};

struct su_request {
    unsigned uid;
    char name[64];
    int login;
    int keepenv;
    char *shell;
    char *command;
    char **argv;
    int argc;
    int optind;
};

struct su_user_info {
    // the user in android userspace (multiuser)
    // that invoked this action.
    unsigned android_user_id;
    // how su behaves with multiuser. see enum below.
    int multiuser_mode;
    // path to superuser directory. this is populated according
    // to the multiuser mode.
    // this is used to check uid/gid for protecting socket.
    // this is used instead of database, as it is more likely
    // to exist. db will not exist if su has never launched.
    char base_path[PATH_MAX];
};

struct su_context {
    struct su_initiator from;
    struct su_request to;
    struct su_user_info user;
    mode_t umask;
    char sock_path[PATH_MAX];
};

// multiuser su behavior
typedef enum {
  // only owner can su
  MULTIUSER_MODE_OWNER_ONLY = 0,
  // owner gets a su prompt
  MULTIUSER_MODE_OWNER_MANAGED = 1,
  // user gets a su prompt
  MULTIUSER_MODE_USER = 2,
  MULTIUSER_MODE_NONE = 3,
} multiuser_mode_t;

#define MULTIUSER_VALUE_OWNER_ONLY    "owner"
#define MULTIUSER_VALUE_OWNER_MANAGED "managed"
#define MULTIUSER_VALUE_USER          "user"
#define MULTIUSER_VALUE_NONE          "none"

typedef enum {
    PROMPT = -1,
    DENY = 0,
    ALLOW_RESTRICTED = 1,
    ALLOW = 2,
} policy_t;

#define RESULT_ACCEPT				"accept"
#define RESULT_ACCEPT_RESTRICTED	"accept_restricted"
#define RESULT_REJECT					"reject"
#define RESULT_PROMPT				"prompt"

extern int send_request(struct su_context *ctx);
#ifdef MIUI
extern void miui_show_disabled();
#endif

static inline char *get_command(const struct su_request *to)
{
  if (to->command)
    return to->command;
  if (to->shell)
    return to->shell;
  char* ret = to->argv[to->optind];
  if (ret)
    return ret;
  return (char *)DEFAULT_SHELL;
}

int run_daemon();
int connect_daemon(int argc, char *argv[]);
int su_main(int argc, char *argv[], int need_client);
// for when you give zero fucks about the state of the child process.
// this version of fork understands you don't care about the child.
// deadbeat dad fork.
int fork_zero_fucks();

// can't use liblog.so because this is a static binary, so we need
// to implement this ourselves
#include <android/log.h>

#ifndef LOG_NDEBUG
#define LOG_NDEBUG 1
#endif

#if LOG_NDEBUG
#define LOGD(...)   ((void)0)
#define LOGV(...)   ((void)0)
#else
#define LOGD ALOGD
#endif

#include <errno.h>
#include <string.h>
#include <cutils/log.h>
#define PLOGE(fmt,args...) LOGE(fmt " failed with %d: %s", ##args, errno, strerror(errno))
#define PLOGEV(fmt,err,args...) LOGE(fmt " failed with %d: %s", ##args, err, strerror(err))

#define LOGE ALOGE
#define LOGW ALOGW
#define LOGI ALOGI

#ifdef __cplusplus
}
#endif

#endif
