/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _CMDTOOL_H_
#define _CMDTOOL_H_

#include <utils/Vector.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <utils/String16.h>

namespace android{

    class CmdTool {
    public:
        /* forks a command and waits for it to finish*/
        int run_command_lite(String16 action, String16 param, int timeout);
        int run_command_lite(const char* action, const char* param, const int timeout=60);
    private:
        bool waitpid_with_timeout_lite(pid_t pid, int timeout_seconds, int* status);
    };
}

#endif /* _CMDTOOL_H_ */
