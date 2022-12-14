/*
 * Copyright (C) 2013 Xiaomi Corporation
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
#include "OutputMethod.h"
#define STRING_LEN 16
#define MAX_DAEMON_CMD_LEN 64
#define AD_SOCKET_TRY_TIMES 10
#define MAX_DAEMON_REPLY_LEN 64
#define AD_SOCKET_WAIT_TIME_SECONDS 6

#define DAEMON_SOCKET "pps"
#define LOG_TAG "DisplayFeatureHal"
#define LOG_NDEBUG 0

#define PPD_STATUS_PROP "init.svc.ppd"
#define TEMP_BUILD

#ifndef TEMP_BUILD
#ifndef PLATFORM_NON_QCOM
using ::vendor::display::postproc::V1_0::IDisplayPostproc;
using ::vendor::display::postproc::V1_0::Result;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_version;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_handle;
#endif
#endif
using std::string;

namespace android {

static int isValid(int Value)
{
    return (Value >= 0) && (Value <= 255);
}

static int writeSockCommand(int daemon_socket, const char* cmd, char* reply)
{
    int ret;
    fd_set fds;
    struct timeval timeout = {5, 0};
    int maxfd;

    DF_LOGD("set function enable:%s", cmd);
    if (TEMP_FAILURE_RETRY(write(daemon_socket, cmd, strlen(cmd))) != (int)strlen(cmd)) {
       DF_LOGE("%s: Failed to send data over socket %s", strerror(errno), DAEMON_SOCKET);
       return -1;
    }

    FD_ZERO(&fds);
    FD_SET(daemon_socket, &fds);
    maxfd = daemon_socket + 1;
    ret = select(maxfd, &fds, NULL, NULL, &timeout);
    if (ret == 0) {//timeout
        DF_LOGE("%s: select socket timeout on %s", strerror(errno), DAEMON_SOCKET);
        return -1;
    } else if (ret < 0) {
        DF_LOGE("%s: select socket error on %s", strerror(errno), DAEMON_SOCKET);
        return -1;
    } else {
        if (!FD_ISSET(daemon_socket, &fds)) {
            DF_LOGE("select socket error not set daemonsocket on %s", DAEMON_SOCKET);
            return -1;
        }
    }

    /* listen for daemon responses */
    DF_LOGD("read respond for:%s", cmd);
    if (TEMP_FAILURE_RETRY(read(daemon_socket, (void*)reply, MAX_DAEMON_REPLY_LEN)) <= 0) {
        DF_LOGE("%s: Failed to get data over socket %s", strerror(errno),
             DAEMON_SOCKET);
        return -1;
    }
    DF_LOGD("Daemon response: %s", reply);
    return 0;
}

#ifndef PPD_STATUS_CHECK_SKIP
static bool isRunPpdService()
{
  bool result = false;
  static char propertyValue[PROPERTY_VALUE_MAX];

  if(property_get(PPD_STATUS_PROP, propertyValue, NULL) > 0) {
    if(!(strncmp(propertyValue, "running", PROPERTY_VALUE_MAX)))
      result = true;
    else
      result = false;
  } else {
    result = false;
  }
  return result;
}
#endif

OutputMethod::OutputMethod()
{
}

OutputMethod::~OutputMethod()
{
}

int OutputMethod::output(OutputParam param)
{
    return 0;
}

void OutputMethod::setParam(void* params)
{
    return;
}

void OutputMethod::dump(std::string& result)
{
    return;
}

int OutputMethod::doDisable()
{
    DF_LOGD("doDisable");
    return 0;
}

AdModeImpl::AdModeImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    DF_LOGD("AD_Impl created");
}

AdModeImpl::~AdModeImpl()
{
    DF_LOGD("AD_Impl destroyed");
}

int AdModeImpl::output(OutputParam param)
{
    return 0;
}

void AdModeImpl::setParam(void* params)
{
    return;
}

SviModeImpl::SviModeImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    DF_LOGD("SviModeImpl created");
}

SviModeImpl::~SviModeImpl()
{
    DF_LOGD("SviModeImpl destroyed");
}

int SviModeImpl::output(OutputParam param)
{
    char cmd[MAX_DAEMON_CMD_LEN] = "";
    char reply[MAX_DAEMON_REPLY_LEN] = "";
    char propertyValue[PROPERTY_VALUE_MAX];
    int adValue = 0;
    int daemon_socket = -1;
    int i;
    int ret;

    DF_LOGD("start config SviServer...");

    /*try to connect to server*/
    for (i = 0; i < AD_SOCKET_TRY_TIMES; i++) {
        daemon_socket = socket_local_client(DAEMON_SOCKET,
                    ANDROID_SOCKET_NAMESPACE_RESERVED,
                    SOCK_STREAM);
        if (daemon_socket >= 0) {
            break;
        }
        sleep(AD_SOCKET_WAIT_TIME_SECONDS);
    }

    if (daemon_socket < 0) {
        DF_LOGE("socket create error, socketid:%d, error:%s", daemon_socket, strerror(errno));
        return -1;
    }

    if (!param.param1) {
        strcpy(cmd, "svi:off");
    } else {
        strcpy(cmd, "svi:on;1");
    }

    ret = writeSockCommand(daemon_socket, cmd, reply);
    close(daemon_socket);
    DF_LOGD("all things done, result:%d", ret);
    return ret;
}

void SviModeImpl::setParam(void* params)
{
    return;
}

PanelModeImpl::PanelModeImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    miDDM = new DisplayDeviceManager();
    DF_LOGD("Panel_Impl created");
}

PanelModeImpl::~PanelModeImpl()
{
    DF_LOGD("PanelModeOutputMethod destroyed");
}

int PanelModeImpl::output(OutputParam param)
{
    struct disp_feature_req feature_req;
    struct disp_doze_brightness_req doze_brightness_req;
    struct disp_dsi_cmd_req dsi_cmd_req;

    if (param.function_id == DISP_FEATURE_DOZE_BRIGHTNESS) {
        doze_brightness_req.base.disp_id = mDisplayId;
        doze_brightness_req.base.flag = MI_DISP_FLAG_NONBLOCK;
        doze_brightness_req.doze_brightness = param.param1;
        if (miDDM->setDozeBrightness(&doze_brightness_req)) {
            DF_LOGE("setDozeBrightness fail value:%d", param.param1);
            return -1;
        }
    } else if (param.function_id == DISP_FEATURE_CRC && param.param1 != FEATURE_OFF) {
        dsi_cmd_req.base.flag = 0;
        dsi_cmd_req.base.disp_id = mDisplayId;
        dsi_cmd_req.tx_state = param.param1;
        dsi_cmd_req.tx_len = param.param2;
        dsi_cmd_req.tx_ptr = (__u64)param.payload;
        if (miDDM->writeDsiCmd(&dsi_cmd_req)) {
            DF_LOGE("writeDsiCmd fail");
            return -1;
        }
    } else {
        feature_req.base.disp_id = mDisplayId;
        feature_req.feature_id = param.function_id;
        feature_req.feature_val = param.param1;
        feature_req.tx_len = param.len;
        feature_req.tx_ptr = (uint64_t)param.payload;
        if (miDDM->setFeature(&feature_req)) {
            DF_LOGE("setDispParam fail value:%d", param.function_id);
            return -1;
        }
    }
    return 0;
}

void PanelModeImpl::setParam(void* params)
{
    return;
}

ColorModeImpl::ColorModeImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    DF_LOGD("QC_Impl created");
    mDisplayColorManager = new DisplayColorManager(display_id);
}

ColorModeImpl::~ColorModeImpl()
{
    DF_LOGD("ColorMode_Impl destroyed");
}

int ColorModeImpl::output(OutputParam param)
{
    struct DispPPParam pSDPPParams;
    int ret = 0;
    if (mDisplayColorManager.get()) {
        pSDPPParams.function_id = param.function_id;
        pSDPPParams.param1 = param.param1;
        pSDPPParams.param2 = param.param2;
        if (param.len && param.payload) {
            pSDPPParams.len = param.len;
            pSDPPParams.payload = param.payload;
        }
        ret = mDisplayColorManager->setColorModeWithRenderIntent(&pSDPPParams);
    }

    return ret;
}

void ColorModeImpl::setParam(void* params)
{
    return;
}

PccModeImpl::PccModeImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    DF_LOGD("Enter");
    mDisplayPostProc = new DisplayPostprocessing(display_id);
}

PccModeImpl::~PccModeImpl()
{
    DF_LOGD("Exit");
}

int PccModeImpl::output(OutputParam param)
{
    int ret = 0;
    struct DispPPParam pSDPPParams;

    if (mDisplayPostProc.get()) {
        pSDPPParams.function_id = param.function_id;
        pSDPPParams.param1 = param.param1;
        pSDPPParams.param2 = param.param2;
        if (param.len && param.payload) {
            pSDPPParams.len = param.len;
            pSDPPParams.payload = param.payload;
        }
        if (param.vec_payload.size() != 0) {
            pSDPPParams.vec_payload.assign(param.vec_payload.begin(), param.vec_payload.end());
        }
        if (param.pcc_payload.size() != 0) {
            pSDPPParams.pcc_payload.assign(param.pcc_payload.begin(), param.pcc_payload.end());
        }
        ret = mDisplayPostProc->setPostprocessingMode(&pSDPPParams);

        if (DISPLAY_FEATURE_HIST == param.function_id
            && (SRE_FEATURE == param.param2 || GAME_SRE_FEATURE == param.param2 ||
                    VSDR2HDR_FEATURE == param.param2 || IGC_DITHER == param.param2)) {
            ret = pSDPPParams.len;
        }
    }

    return ret;
}
void PccModeImpl::setParam(void* params)
{
    if (mDisplayPostProc.get()) {
        mDisplayPostProc->setParams(params);
    }

    return;
}

void PccModeImpl::dump(std::string& result)
{
    if (mDisplayPostProc.get()) {
        mDisplayPostProc->dumpPCCParam(result);
    }

    return;
}

DSPPModeImpl::DSPPModeImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    DF_LOGD("DSPP Impl created");
    mDisplayColorManager = new DisplayColorManager(display_id);
}

DSPPModeImpl::~DSPPModeImpl()
{
    DF_LOGD("DSPP Impl destroyed");
}

int DSPPModeImpl::output(OutputParam param)
{
    struct DispPPParam pSDPPParams;
    int ret = 0;

    if (mDisplayColorManager.get()) {
        pSDPPParams.function_id = param.function_id;
        pSDPPParams.param1 = param.param1;
        pSDPPParams.param2 = param.param2;
        if (param.len && param.payload) {
            pSDPPParams.len = param.len;
            pSDPPParams.payload = param.payload;
        }
        if (param.vec_payload.size()) {
            pSDPPParams.vec_payload.assign(param.vec_payload.begin(), param.vec_payload.end());
        }
        ret = mDisplayColorManager->setColorService(&pSDPPParams);
    }

    return ret;
}

void DSPPModeImpl::setParam(void* params)
{
    if (mDisplayColorManager.get()) {
        if (params != NULL)
            mDisplayColorManager->setParams(params);
    }
    return;
}

MTKModeImpl::MTKModeImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    DF_LOGD("Enter");
    mDisplayPostProc = new DisplayPostprocessing(display_id);
}

MTKModeImpl::~MTKModeImpl()
{
    DF_LOGD("Exit");
}

int MTKModeImpl::output(OutputParam param)
{
    int ret = 0;
    struct DispPPParam pSDPPParams;

    if (mDisplayPostProc.get()) {
         pSDPPParams.function_id = param.function_id;
        pSDPPParams.param1 = param.param1;
        pSDPPParams.param2 = param.param2;
        if (param.len && param.payload) {
            pSDPPParams.len = param.len;
            pSDPPParams.payload = param.payload;
        }
        ret = mDisplayPostProc->setPostprocessingMode(&pSDPPParams);
    }

    return ret;
}
void MTKModeImpl::setParam(void* params)
{
    if (params != NULL && mDisplayPostProc.get())
        mDisplayPostProc->setParams(params);
    return;
}

LtmModeImpl::LtmModeImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    DF_LOGD("LTM_Impl created");
}

LtmModeImpl::~LtmModeImpl()
{
    DF_LOGD("LTM_Impl destroyed");
}

int LtmModeImpl::output(OutputParam param)
{
#if 0
    int ret = 0;
#ifndef PLATFORM_NON_QCOM
#ifdef ENABLE_LTM
    char cmd[MAX_DAEMON_CMD_LEN] = "";
    sp<IDisplayPostproc> sClientPostproc = NULL;

    if (param.value == ltmStatus && param.value != LTM_USERMODE) {
        DF_LOGD("current ltm status %d, incoming ltm cmd %d, return", ltmStatus, param.value);
        return ret;
    }

    //hidl_version version = android::hardware::make_hidl_version(1,0);
    DF_LOGD("Query DisplayPostproc service");

    if(sClientPostproc == NULL) {
        sClientPostproc = IDisplayPostproc::getService();
        if(sClientPostproc == NULL) {
            DF_LOGE("Query mClientPostproc service returned null");
            return -1;
        }
    }

    if (param.value == LTM_OFF) {
        strcpy(cmd, "Ltm:Off:Primary");
    } else if (param.value == LTM_STRENGTH) {
        sprintf(cmd, "Ltm:ConstantAls:Primary:%d", param.cookie);
    } else if (param.value == LTM_USERMODE) {
        DF_LOGD("Ltm User Mode:%s", getLtmUserModeName(param.cookie));
        sprintf(cmd, "Ltm:UserMode:Primary:%s", getLtmUserModeName(param.cookie));
    } else {
        strcpy(cmd, "Ltm:On:Primary:Auto");
    }
    ltmStatus = param.value;
    const hidl_string dppsmsg(cmd);
    string response;

    sClientPostproc->sendDPPSCommand(dppsmsg,
        [&](const auto& tmpReturn, const auto& tmpResponse) {
           if (tmpReturn != vendor::display::postproc::V1_0::Result::OK) {
                DF_LOGE("set LTM failure! %d", tmpReturn);
                ret = -1;
                return;
            }
            response = tmpResponse;
        });
#endif
#endif
#endif
    return 0;
}

void LtmModeImpl::setParam(void* params)
{
    return;
}

BufDumpImpl::BufDumpImpl(unsigned int display_id)
    :mDisplayId(display_id)
{
    DF_LOGD("BufDumpImpl created");
}

BufDumpImpl::~BufDumpImpl()
{
    DF_LOGD("BufDumpImpl destroyed");
}

int BufDumpImpl::output(OutputParam param)
{
#ifdef CWB_SUPPORT
    DisplayBufDump & dbd = DisplayBufDump::getInstance();
    dbd.Dump(param.value, param.cookie);
#endif
    return 0;
}

void BufDumpImpl::setParam(void* params)
{
    return;
}
}
