/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _DISPLAYCOUNT_MI_H_
#define _DISPLAYCOUNT_MI_H_

#include <utils/Log.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <utils/threads.h>
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <mi_disp.h>

#define BUF_LEN (1024*4)
#define MAX_STRING_LEN 1024

static char *DISPLAY_COUNT_FILE[] = {"/data/vendor/display/display_count_data.txt", "/data/vendor/display/display_count_data_sec.txt"};
static char *DISPLAY_COUNT_SYS_FILE[] = {"/sys/class/mi_display/disp-DSI-0/disp_count", "/sys/class/mi_display/disp-DSI-1/disp_count"};
static char DISP_FEATURE_DEVICE_PATH[] = "/dev/mi_display/disp_feature";

namespace android {
class DisplayCountThread : public Thread {
public:
    DisplayCountThread(int);

protected:
    virtual ~DisplayCountThread();

private:
    static void *ThreadWrapperDisplayCount(void *);
    virtual bool threadLoop();
    virtual status_t readyToRun();

    void SaveDispayCount(void);
    void restoreDisplayCount(void);

    int registerPollEvent();

    int mDisplayCountFd[MI_DISP_MAX];
    pollfd mPollFd;
    bool mRestore;
    int mDisplayId;
    pthread_t mThreadDisplayCount;
    bool mDisplaycountPoll;
};
}
#endif
