#define LOG_NDEBUG 0
#define LOG_TAG "NetdConstantsExtend"

#include <string>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#include <list>

#include <cutils/log.h>
#include <logwrap/logwrap.h>

#include "NetdConstantsExtend.h"

/*
const char * const IPTABLES_PATH = "/system/bin/iptables";
const char * const IP6TABLES_PATH = "/system/bin/ip6tables";

static void logExecError(const char* argv[], int res, int status) {
    const char** argp = argv;
    std::string args = "";
    while (*argp) {
        args += *argp;
        args += ' ';
        argp++;
    }
    ALOGE("exec() res=%d, status=%d for %s", res, status, args.c_str());
}
*/

int execIptablesRestoreExtend(IptablesTarget target, const std::string& commands) {
    (void) target; // unused target
    (void) commands; // unused commands
    return 0;
}

int enableIptablesRestore(bool enabled) {
    (void) enabled; // unused enabled
    return 0;
}

