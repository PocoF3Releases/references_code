/*
 * Copyright (c) 2017 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include "TouchFeature.h"
#include <linux/ioctl.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#undef LOG_TAG
#define LOG_TAG "TouchFeatureV2"

#define TOUCH_NODE "/dev/xiaomi-touch"
#define TOUCH_SOCKET "touchevent"

#define CMD_LEN	4
#define LONG_CMD_LEN 256
#define MESSAGE_MAX_LEN 1024

using namespace android;
using std::string;

namespace vendor {
namespace xiaomi {
namespace hw {
namespace touchfeature {
namespace V1_0 {
namespace implementation {

enum DATA_STRUCT {
    CUR_VALUE = 0,
    DEF_VALUE,
    MIN_VALUE,
    MAX_VALUE,
    TOTAL_NUM,
};

enum MODE_CMD {
    SET_CUR_VALUE = 0,
    GET_CUR_VALUE,
    GET_DEF_VALUE,
    GET_MIN_VALUE,
    GET_MAX_VALUE,
    GET_MODE_VALUE,
    RESET_MODE,
    SET_LONG_VALUE,
};

enum  MODE_TYPE {
    Touch_ALL_MODE        = 0,
    Touch_DOZE_MODE       = 1,
    Touch_UP_THRESHOLD    = 2,
    Touch_LAND_LOCK_PIXEL = 3,
    Touch_LAND_LOCK_TIME  = 4,
    Touch_EDGE_FILTER     = 5,
};

enum {
    CMD_GET_TOUCHEVENT = 1,
    CMD_REPLY = 100,
};

struct  msg_t {
    int cmd;
    int length;
    char message[MESSAGE_MAX_LEN];
};

static int write_touch_device(int32_t touchId, int32_t mode, int32_t value)
{
    int fd;
    int ret = 0;
    int cmd[CMD_LEN] = {0, };
    fd = open(TOUCH_NODE, O_RDWR);
    if (fd < 0) {
        ALOGE("Can't open touch node:%d", fd);
        return -1;
    }
	cmd[0] = touchId;
    cmd[1] = mode;
    cmd[2] = value;

    ret = ioctl(fd, _IO('T', SET_CUR_VALUE), &cmd);
    close(fd);
    return ret;
}

static int get_touch_device(int32_t touchId, int32_t mode, int32_t value_type)
{
    int fd;
    int ret = 0;
    int cmd[CMD_LEN] = {0, };

    fd = open(TOUCH_NODE, O_RDWR);
    if (fd < 0) {
        ALOGE("Can't open touch node:%d", fd);
        return -1;
    }
	cmd[0] = touchId;
    cmd[1] = value_type;
    ret = ioctl(fd, _IO('T', mode), &cmd);
    close(fd);
    if (ret >= 0)
        ret = cmd[0];

    return ret;
}

static int get_mode_value(int32_t touchId, int32_t mode, hidl_vec<int32_t>& out)
{
    int fd;
    int ret = 0;
    int cmd[CMD_LEN] = {0, };
    fd = open(TOUCH_NODE, O_RDWR);
    if (fd < 0) {
        ALOGE("Can't open touch node:%d", fd);
        return -1;
    }
	cmd[0] = touchId;
    cmd[1] = mode;

    ret = ioctl(fd, _IO('T', GET_MODE_VALUE), &cmd);
    close(fd);
    if (ret >= 0) {
        out[0] = cmd[0];
        out[1] = cmd[1];
        out[2] = cmd[2];
        out[3] = cmd[3];
    }
    return ret;
}

static int sockfd = -1;
static pthread_mutex_t sockfd_mutex;

static int get_sock_state() {
    if (sockfd > 0) {
        struct pollfd fds[1];
        fds[0].fd = sockfd;
        fds[0].events = POLLHUP;
        poll(fds, 1, 0);
        if (fds[0].revents & POLLHUP) {
            ALOGI("peer socket closed\n");
            close(sockfd);
            sockfd = -1;
        }
    }
    return sockfd;
}

static int connect_server(const char *name) {
    struct sockaddr_un addr;
    socklen_t alen = 0;
    int fd, ret = 0;

    if (name == NULL) {
        ALOGE("%s NULL\n", __func__);
        return -1;
    }
    fd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0) {
        ALOGE("socket error:%s\n", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path + 1, name, strlen(name));
    alen = offsetof(struct sockaddr_un, sun_path) + strlen(name) + 1;

    ret = connect(fd, (struct sockaddr *) &addr, alen);
    if (ret < 0) {
        ALOGE("connect failed, socket:%d, error:%s\n", fd, strerror(errno));
        close(fd);
        return -1;
    }

    ALOGD("connect success, socket:%d\n", fd);
    return fd;
}

static int get_sockfd() {
    pthread_mutex_lock(&sockfd_mutex);
    if (get_sock_state() == -1) {
        sockfd = connect_server(TOUCH_SOCKET);
    }
    pthread_mutex_unlock(&sockfd_mutex);
    return sockfd;
}

static void close_socket() {
    ALOGI("%s\n", __func__);
    pthread_mutex_lock(&sockfd_mutex);
    if (get_sock_state() > 0) {
        close(sockfd);
        sockfd = -1;
    }
    pthread_mutex_unlock(&sockfd_mutex);
}

TouchFeature::TouchFeature()
{
    ALOGD("HW TouchFeature contruct");
}

TouchFeature::~TouchFeature()
{
    ALOGD("HW TouchFeature destroy");
    close_socket();
}

// Methods from ::vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature follow.
Return<int32_t> TouchFeature::setModeValue(int32_t touchId, int32_t ControlMode, int32_t ModeValue)
{
    // TODO implement

    if (ControlMode == 100) {
        ALOGI("set persist.vendor.hostprocess.waterproof to %d", ModeValue);
        property_set("persist.vendor.hostprocess.waterproof", ModeValue == 0 ? "0" : "1");
    }

    ALOGD("setModeValue id:%d, mode:%d, value:%d", touchId, ControlMode, ModeValue);

    return write_touch_device(touchId, ControlMode, ModeValue);
}

Return<int32_t> TouchFeature::getModeCurValue(int32_t touchId, int32_t ControlMode)
{
    // TODO implement
    ALOGD("getModeCurValue id:%d, mode:%d", touchId, ControlMode);

    return get_touch_device(touchId, GET_CUR_VALUE, ControlMode);
}

Return<int32_t> TouchFeature::getModeMinValue(int32_t touchId, int32_t ControlMode)
{
    // TODO implement
    ALOGD("getModeMinValue id:%d, mode:%d", touchId, ControlMode);

    return get_touch_device(touchId, GET_MIN_VALUE, ControlMode);
}

Return<int32_t> TouchFeature::getModeDefaultValue(int32_t touchId, int32_t ControlMode)
{
    // TODO implement
    ALOGD("getModeDefaultValue id:%d, mode:%d", touchId, ControlMode);

    return get_touch_device(touchId, GET_DEF_VALUE, ControlMode);
}

Return<int32_t> TouchFeature::getModeMaxValue(int32_t touchId, int32_t ControlMode)
{
    // TODO implement
    ALOGD("getModeMaxValue id:%d, mode:%d", touchId, ControlMode);

    return get_touch_device(touchId, GET_MAX_VALUE, ControlMode);
}

Return<int32_t> TouchFeature::modeReset(int32_t touchId, int32_t ControlMode)
{
    // TODO implement
    ALOGD("modeReset id:%d, mode:%d", touchId, ControlMode);

	return get_touch_device(touchId, RESET_MODE, ControlMode);
}

Return<void> TouchFeature::getModeValue(int32_t touchId, int32_t mode, getModeValue_cb _hidl_cb)
{
    // TODO implement
    int32_t ret = 0;
    ALOGD("id:%d, getModeValue:%d", touchId, mode);
    hidl_vec<int32_t> out;

    out.resize(TOTAL_NUM);

    ret = get_mode_value(touchId, mode, out);
    ALOGD("getModeValue:%d, %d, %d, %d, %d", mode, out[0], out[1], out[2], out[3]);
    _hidl_cb(out);

    return Void();
}

Return<int32_t> TouchFeature::setModeLongValue(int32_t touchId, int32_t ControlMode, int32_t ValueLen, const hidl_vec<int32_t>& ValueBuf)
{
    int fd;
    int ret = 0;
    int cmdbuf[LONG_CMD_LEN] = {0, };

    cmdbuf[0] = touchId;
    cmdbuf[1] = ControlMode;
    cmdbuf[2] = ValueLen;

    if (ValueLen > LONG_CMD_LEN - 3) {
        ALOGE("id:%d, mode value is too long:%d", touchId, ValueLen);
        return -1;
    }

    for (size_t i = 0; i < ValueBuf.size() && i < ValueLen; i++) {
        *(cmdbuf + i + 3) = ValueBuf[i];
    }

    fd = open(TOUCH_NODE, O_RDWR);
    if (fd < 0) {
        ALOGE("Can't open touch node:%d", fd);
        return -1;
    }


    ret = ioctl(fd, _IO('T', SET_LONG_VALUE), &cmdbuf);
    close(fd);
    return ret;
}

Return<void> TouchFeature::getTouchEvent(getTouchEvent_cb _hidl_cb) {
    int fd = get_sockfd();
    int ret;
    msg_t msg;

    memset(&msg, 0, sizeof(msg));
    if (fd > 0) {
        msg.cmd = CMD_GET_TOUCHEVENT;
        ret = send(fd, &msg, sizeof(msg), 0);
        if (ret <= 0) {
            ALOGE("write socket %d failed, error: %s", fd, strerror(errno));
            msg.length = 0;
        } else {
            ret = recv(fd, &msg, sizeof(msg), 0);
            if (ret <= 0) {
                ALOGE("read socket %d failed, error: %s", fd, strerror(errno));
                msg.length = 0;
            }
        }
    }

    string out(msg.message, 0, msg.length);
    _hidl_cb(out);

    return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

ITouchFeature* HIDL_FETCH_ITouchFeature(const char* /* name */) {

    return new TouchFeature();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace touchfeature
}  // namespace hw
}  // namespace xiaomi
}  // namespace vendor
