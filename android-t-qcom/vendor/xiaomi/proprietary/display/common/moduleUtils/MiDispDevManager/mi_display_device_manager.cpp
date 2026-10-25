/*
 * Copyright (C) 2020 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>

#include <utils/Log.h>
#include "mi_display_device_manager.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "MiDisplayDM"
#endif
#define MI_DEVICE "/dev/mi_display/disp_feature"

namespace android {


DisplayDeviceManager::DisplayDeviceManager()
    :mMiDeviceFd(-1)
{
    ALOGD("%s", __func__);
    mMiDeviceFd = open(MI_DEVICE, O_RDWR | O_CLOEXEC);
    if(mMiDeviceFd < 0) {
        ALOGE("could not open %s\n", MI_DEVICE);
    }
}

DisplayDeviceManager::~DisplayDeviceManager()
{
    ALOGD("%s", __func__);
    if(mMiDeviceFd >= 0) close(mMiDeviceFd);
}

int DisplayDeviceManager::setFeature(struct disp_feature_req *feature_req)
{
    int ret = -1;

    if (!feature_req || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    ALOGD("%s, feature_id:%d\n", __func__, feature_req->feature_id);
    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_SET_FEATURE, feature_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    return ret;
}

int DisplayDeviceManager::setDozeBrightness(struct disp_doze_brightness_req *doze_req)
{
    int ret = -1;

    if (!doze_req || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    ALOGD("%s, doze brightness:%d\n", __func__, doze_req->doze_brightness);
    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_SET_DOZE_BRIGHTNESS, doze_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    return ret;
}

int DisplayDeviceManager::getDozeBrightness(struct disp_doze_brightness_req *doze_req)
{
    int ret = -1;

    if (!doze_req || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    doze_req->doze_brightness = 0;
    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_GET_DOZE_BRIGHTNESS, doze_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    ALOGD("%s, doze brightness:%d\n", __func__, doze_req->doze_brightness);
    return ret;
}

int DisplayDeviceManager::getPanelInfo(struct disp_panel_info *panel_info_req)
{
    int ret = -1;

    if (!panel_info_req || !panel_info_req->info || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_GET_PANEL_INFO, panel_info_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    ALOGD("%s, doze brightness:%s\n", __func__, panel_info_req->info);
    return ret;
}

int DisplayDeviceManager::getWPInfo(struct disp_wp_info *wp_info_req)
{
    int ret = -1;

    if (!wp_info_req || !wp_info_req->info || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_GET_WP_INFO, wp_info_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    ALOGD("%s, doze brightness:%s\n", __func__, wp_info_req->info);
    return ret;

}

int DisplayDeviceManager::getFPS(struct disp_fps_info *fps_info_req)
{
    int ret = -1;

    if (!fps_info_req || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    fps_info_req->fps = 0;
    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_GET_FPS, fps_info_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    ALOGD("%s, doze brightness:%d\n", __func__, fps_info_req->fps);
    return ret;
}

int DisplayDeviceManager::registerEvent(struct disp_event_req *event_req)
{
    int ret = -1;

    if (!event_req || !isSupportDispEventType(event_req->type) || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    ALOGD("%s, event type name: %s\n", __func__, getDispEventTypeName(event_req->type));
    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_REGISTER_EVENT, event_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    return 0;
}

int DisplayDeviceManager::deRegisterEvent(struct disp_event_req *event_req)
{
    int ret = -1;

    if (!event_req || !isSupportDispEventType(event_req->type) || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    ALOGD("%s, event type name: %s\n", __func__, getDispEventTypeName(event_req->type));
    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_DEREGISTER_EVENT, event_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    return 0;
}

int DisplayDeviceManager::writeDsiCmd(struct disp_dsi_cmd_req* dsi_cmd_req)
{
    int ret = -1;
    if (!dsi_cmd_req || mMiDeviceFd < 0) {
        ALOGD("%s, invalid parameter\n", __func__);
        return ret;
    }

    ret = ioctl(mMiDeviceFd, MI_DISP_IOCTL_WRITE_DSI_CMD, dsi_cmd_req);
    if(ret) {
        ALOGE("%s, ioctl fail\n", __func__);
    }

    return 0;

}
}
