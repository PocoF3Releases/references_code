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
#include "displaycount.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "displaycount"
#endif

using namespace std;

#define BUF_LEN (1024*4)
#define MAX_STRING_LEN 1024

namespace android {

DisplayCountThread::DisplayCountThread(int mDisplayId)
    : mDisplayCountFd{-1,-1}
    , mDisplayId(mDisplayId)
    , mRestore(false)
    , mDisplaycountPoll(true) {
    mPollFd.fd = -1;
    readyToRun();
    pthread_create(&mThreadDisplayCount, NULL, ThreadWrapperDisplayCount, this);
}

void *DisplayCountThread::ThreadWrapperDisplayCount(void *context) {
    if (context) {
        return (void *)(uintptr_t)static_cast<DisplayCountThread *>(context)->threadLoop();
    }
    return NULL;
}

DisplayCountThread::~DisplayCountThread() {
    if (mDisplayCountFd[mDisplayId] >= 0) {
        close(mDisplayCountFd[mDisplayId]);
    }

    if (mDisplaycountPoll) {
        pthread_join(mThreadDisplayCount, NULL);
        mThreadDisplayCount = 0;
        close(mPollFd.fd);
        mPollFd.fd = -1;
        mDisplaycountPoll = false;
    }

}

void DisplayCountThread::SaveDispayCount(void)
{
    int ret = -1;
    char data[BUF_LEN] = {0};
    int fdsys;
    /*TODO: temp sdsys*/
    fdsys = open(DISPLAY_COUNT_SYS_FILE[mDisplayId], O_RDWR);
    if (fdsys < 0) {
        ALOGE("open %s failed, error = %s\n", DISPLAY_COUNT_SYS_FILE[mDisplayId], strerror(errno));
        return;
    }
    memset(data, 0, sizeof(data));
    ret = pread(fdsys, data, sizeof(data), 0);
    if (ret < 0) {
        ALOGE("read %s failed, error = %s\n", DISPLAY_COUNT_SYS_FILE[mDisplayId], strerror(errno));
        close(fdsys);
        return;
    }
    close(fdsys);

    ret = pwrite(mDisplayCountFd[mDisplayId], data, strlen(data), 0);
    if (ret < 0)
        ALOGE("write %s failed, error = %s\n", DISPLAY_COUNT_FILE[mDisplayId], strerror(errno));

    return;
}

void DisplayCountThread::restoreDisplayCount(void)
{
    int ret = -1;
    char data[BUF_LEN] = {0};
    int fdsys;

    memset(data, 0, sizeof(data));
    ret = pread(mDisplayCountFd[mDisplayId], data, sizeof(data), 0);
    if (ret < 0) {
        ALOGE("restoreDisplayCount read %s failed, error = %s\n", DISPLAY_COUNT_FILE[mDisplayId], strerror(errno));
        return;
    }
    /* TODO: temp sdsys */
    fdsys = open(DISPLAY_COUNT_SYS_FILE[mDisplayId], O_RDWR);
    if (fdsys < 0) {
        ALOGE("restoreDisplayCount open %s failed,error = %s", DISPLAY_COUNT_SYS_FILE[mDisplayId], strerror(errno));
        return;
    }
    ret = pwrite(fdsys, data, strlen(data), 0);
    if (ret < 0)
        ALOGE("restoreDisplayCount write %s failed,error = %s", DISPLAY_COUNT_SYS_FILE[mDisplayId], strerror(errno));
    close(fdsys);

    return;
}

status_t DisplayCountThread::readyToRun() {
    int fdsys = 0;
    mDisplayCountFd[mDisplayId] = open(DISPLAY_COUNT_FILE[mDisplayId], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (mDisplayCountFd[mDisplayId] < 0) {
        ALOGE("readyToRun open %s failed, error = %s\n", DISPLAY_COUNT_FILE[mDisplayId], strerror(errno));
        return NO_INIT;
    }

    if (!mRestore) {
        restoreDisplayCount();
        mRestore = true;
    }

    if (registerPollEvent()) {
        ALOGE("register poll event failed");
        return NO_INIT;
    }

    return NO_ERROR;
}

int DisplayCountThread::registerPollEvent()
{
    int ret = 0;
    struct disp_event_req event_req;

    mPollFd.fd = open(DISP_FEATURE_DEVICE_PATH, O_RDONLY);
    if (mPollFd.fd < 0) {
        ALOGE("registerPollEvent open %s failed", DISP_FEATURE_DEVICE_PATH);
        return -1;
    } else {
        ALOGD("open %s success", DISP_FEATURE_DEVICE_PATH);
    }
    mPollFd.events = POLLIN | POLLPRI | POLLERR;

    event_req.base.disp_id = mDisplayId;
    event_req.type = MI_DISP_EVENT_POWER;
    ret = ioctl(mPollFd.fd, MI_DISP_IOCTL_REGISTER_EVENT, &event_req);
    if (ret) {
        ALOGE("%s, ioctl MI_DISP_IOCTL_REGISTER_EVENT fail.\n", __func__);
        return -1;
    }

    return 0;
}

bool DisplayCountThread::threadLoop()
{
    struct disp_event_resp *event_resp = NULL;
    char event_data[MAX_STRING_LEN] = {0};
    int32_t size;

    while(mDisplaycountPoll) {
        int pollResult = poll(&mPollFd, 1, -1);

        if (pollResult == 0) {
            ALOGE("pollResult == 0");
            return true;
        } else if (pollResult < 0) {
            ALOGE("Could not poll events");
            return true;
        }

        if (mPollFd.fd < 0) {
            return true;
        }

        if (mPollFd.revents & (POLLIN | POLLPRI | POLLERR)) {
            size = read(mPollFd.fd, event_data, MAX_STRING_LEN);
            if (size < 0) {
                ALOGE("read disp feature event failed.\n");
            }

            if (size > MAX_STRING_LEN) {
                ALOGE("event size %d is greater than event buffer size %d\n", size, MAX_STRING_LEN);
                return true;
            }

            if (size < (int32_t)sizeof(*event_resp)) {
                ALOGE("size %d exp %zd\n", size, sizeof(*event_resp));
                return true;
            }

            // event_resp = (struct disp_event_resp *)&event_data;

            int32_t i = 0;
            while (i < size) {
                event_resp = (struct disp_event_resp *)&event_data[i];
                switch (event_resp->base.type) {
                    case MI_DISP_EVENT_POWER:
                    {
                        uint32_t* event_payload = reinterpret_cast<uint32_t *>(event_resp->data);
                        if (*event_payload == 0) {
                            ALOGD("Received Idle power %d", *event_payload);
                            SaveDispayCount();
                        }
                        break;
                    }
                    default: {
                        ALOGE("invalid event %d", event_resp->base.type);
                        break;
                    }
                }
                i += event_resp->base.length;
            }
        }
    }
    pthread_exit(0);
    return true;
}
} //namespace android
