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

#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Singleton.h>
#include <utils/SortedVector.h>
#include <utils/String8.h>
#include <sys/prctl.h>
#include <errno.h>
#include <assert.h>
#include <sys/poll.h>

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <cutils/properties.h>
#include <fstream>

#include "display_property.h"
#include "DisplayFeatureHal.h"
#include "DisplayPostprocessing.h"
#include "XmlParser.h"
#include "timer.hpp"
#include "DisplayEffect.h"
#include "cust_define.h"
#define LOG_TAG "DisplayFeatureHal"

using std::fstream;
#ifndef MTK_PLATFORM
using snapdragoncolor::IMiStcService;
#endif
namespace android {

int DisplayEffectBase::getPanelName(const char *file_name,char *panel_name)
{
    char _panel_name[256] = {0};// Panel name
    XmlParser xml_handle;
    fstream fs;
    fs.open(file_name,fstream::in);
    if (!fs.is_open()){
        DF_LOGE("Failed to open msm_fb_panel_info node device node\n");
        return -1;
    }
    string line;
    while (std::getline(fs, line)) {
        unsigned int token_count = 0;
        const unsigned int max_count = 10;
        char *tokens[max_count] = {NULL};
        if (!xml_handle.ParseLine(line.c_str(), "=\n", tokens, max_count, &token_count)) {
            DF_LOGV("%s\t%s\t%d\n",tokens[0],tokens[1],token_count);
            if(tokens[0] != NULL)
            if (!strncmp(tokens[0], "panel_name", strlen("panel_name"))) {

                snprintf(_panel_name, sizeof(_panel_name), "%s", tokens[1]);
                break;
            }
        }
    }

    if (strlen(_panel_name)) {
        snprintf(&panel_name[0], sizeof(_panel_name), "%s", &_panel_name[0]);
        char *tmp = panel_name;
        while ((tmp = strstr(tmp, " ")) != NULL)
            *tmp = '_';
        if ((tmp = strstr(_panel_name, "\n")) != NULL)
            *tmp = '\0';
        //DF_LOGV("panel_name:%s\n",panel_name);
    }

    return 0;
}

int DisplayEffectBase::parseXml()
{
    //parse xml
    int result = 0;
    int err = 0;
    XmlParser xml_handle;
    string file_name = mDisplayId == DISPLAY_PRIMARY ? PRIM_PANEL_INFO_PATH : SEC_PANEL_INFO_PATH;
    char panel_name[255];
    char temp[50];
    long maxBrightness;
    int realBrightness;
    char property_value[PROPERTY_VALUE_MAX];
    HBMConfigParams hbm_params;
    struct OutputParam sParams;

    getPanelName(file_name.c_str(),panel_name);
    strcat(panel_name,"_mi.xml");
    string fName;
    fName.append("odm/etc/");
    fName.append(panel_name);

    string key("strength");
    char p[4096];
    unsigned int token_count = 0;
    const unsigned int max_count = 200;
    char *tokens[max_count] = {NULL};
    err = xml_handle.parseXml(fName);

    if(err >= 0) {
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mPccParams.uctParam.strength = atoi(p);
            DF_LOGV("Unlimited CT strength:%d",atoi(p));
        }
        key = "default";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<3;i++) {
                    mPccParams.uctParam.def[i] = atoi(tokens[i]);
                    DF_LOGV("mUCTParams default[%d]=%d ",i,mPccParams.uctParam.def[i]);
                }
            }
        }
        key = "cold";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<3;i++) {
                    mPccParams.uctParam.cold[i] = atoi(tokens[i]);
                    DF_LOGV("mUCTParams cold[%d]=%d ",i,mPccParams.uctParam.cold[i]);
                }
            }
        }
        key = "warm";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<3;i++) {
                    mPccParams.uctParam.warm[i] = atoi(tokens[i]);
                    DF_LOGV("mUCTParams warm[%d]=%d ",i,mPccParams.uctParam.warm[i]);
                }
            }
        }
        key = "uiParamMin";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mPccParams.uctParam.uiParamMin = atoi(p);
            DF_LOGV("uiParam min:%d",atoi(p));
        }
        key = "paramMax";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mPccParams.uctParam.paramMax = atoi(p);
            DF_LOGV("param max:%d",atoi(p));
        }
        key = "dimmingTime";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mPccParams.uctParam.dimmingTime = atoi(p);
            DF_LOGV("dimming time:%d",atoi(p));
        }
        key = "dimmingStep";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mPccParams.uctParam.dimmingStep = atoi(p);
            DF_LOGV("dimming step:%d",atoi(p));
        }
        //wp calibration
        key = "targetWP";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<2;i++) {
                    mPccParams.wpCalibParam.targetWP[i] = atof(tokens[i]);
                    DF_LOGV("WP calibration target WP %d %f ",i,mPccParams.wpCalibParam.targetWP[i]);
                }
            }
        }
        key = "targetWPXY";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<3;i++) {
                    mPccParams.wpCalibParam.targetXY[i] = atof(tokens[i]);
                    DF_LOGV("WP calibration target XY %d %f ",i,mPccParams.wpCalibParam.targetXY[i]);
                }
            }
        }
        key = "rgbCoordinatexyl";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        mPccParams.wpCalibParam.rgbCoodinatexyl[i][j] = atof(tokens[3*i + j]);
                        DF_LOGV("WP calibration rgbCoodinatexyl %f ",atof(tokens[3*i + j]));
                    }
                }
            }
        }
        key = "xRange";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<2;i++) {
                    mPccParams.wpCalibParam.xRange[i] = atof(tokens[i]);
                    DF_LOGV("WP calibration xRange %d %f ",i,mPccParams.wpCalibParam.xRange[i]);
                }
            }
        }
        key = "yRange";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<2;i++) {
                    mPccParams.wpCalibParam.yRange[i] = atof(tokens[i]);
                    DF_LOGV("WP calibration yRange %d %f ",i,mPccParams.wpCalibParam.yRange[i]);
                }
            }
        }
        key = "xyComp";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<2;i++) {
                    mPccParams.wpCalibParam.xyComp[i] = atof(tokens[i]);
                    DF_LOGV("WP calibration xyComp %d %f ",i,mPccParams.wpCalibParam.xyComp[i]);
                }
            }
        }
        key = "xyOffset";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i=0;i<2;i++) {
                    mPccParams.wpCalibParam.xyOffset[i] = atof(tokens[i]);
                    DF_LOGV("WP calibration xyOffset %d %f ",i,mPccParams.wpCalibParam.xyOffset[i]);
                }
            }
        }
        //max brightness
        key = "maxBrightnessEnable";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mReadMaxBrightnessEnable = atoi(p);
            DF_LOGV("mReadMaxBrightnessEnable:%d",atoi(p));
        }
        key = "typicalBrightness";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mTypBrightness = atoi(p);
            DF_LOGV("typicalBrightness:%d",atoi(p));
        }

        if (mReadMaxBrightnessEnable) {
            key = "startBrightness";
            result = xml_handle.getXmlValue(key,p,sizeof(p));
            if(result >= 0) {
                mStartBrightness = atoi(p);
                DF_LOGV("startBrightness:%d",atoi(p));
            }
            key = "brightnessStep";
            result = xml_handle.getXmlValue(key,p,sizeof(p));
            if(result >= 0) {
                mBrightnessStep = atof(p);
                DF_LOGV("brightnessStep:%f",atof(p));
            }
        }

        //GABB
        key = "luxTable";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i = 0; i < GABB_LUX_TABLE_LEN; i++) {
                    mGABBParams.luxTable[i] = atoi(tokens[i]);
                    DF_LOGV("GABB luxTable[%d]=%d ",i,mGABBParams.luxTable[i]);
                }
            }
        }
        for (int i = 0; i < GABB_LUX_TABLE_LEN; i++) {
            key = "regTable";
            sprintf(temp, "%d", i);
            key.append(temp);
            result = xml_handle.getXmlValue(key,p,sizeof(p));
            if(result >= 0) {
                if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                    for (int j = 0; j < GABB_PARAM_LEN; j++) {
                        mGABBParams.regTable[i][j] = atoi(tokens[j]);
                        DF_LOGV("GABB reg table %d %d %d ",i, j, atoi(tokens[j]));
                    }
                }
            }
        }
        for (int i = 0; i < 5; i++) {
            key = "crcCmd";
            sprintf(temp, "%d", i);
            key.append(temp);
            result = xml_handle.getXmlValue(key,mGABBParams.cmd[i],sizeof(mGABBParams.cmd[i]));
            DF_LOGV("GABB crc cmd %d %s", i, mGABBParams.cmd[i]);
        }
        for (int i = 0; i < 9; i++) {
            key = "pccGABBParams";
            sprintf(temp, "%d", i);
            key.append(temp);
            result = xml_handle.getXmlValue(key,p,sizeof(p));
            if(result >= 0) {
                if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                    for (int j = 0; j < 9; j++) {
                        mPccParams.GABBParams.GABBCoeff[i][j] = atof(tokens[j]);
                        DF_LOGV("pcc GABB params %d %d %f ",i, j, atof(tokens[j]));
                    }
                }
            }
        }
        key = "PccGABBThreshold";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mPccParams.GABBParams.threshold = atoi(p);
            DF_LOGV("pcc GABB threshold:%d",atoi(p));
        }
        //sunlight screen
        key = "level";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mPccParams.slsParams.level = atoi(p);
            DF_LOGV("Sunlight Screen level num:%d",atoi(p));
        }
        key = "delay";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mPccParams.slsParams.delay = atoi(p);
            DF_LOGV("Sunlight Screen delay:%d",atoi(p));
        }
        for (int i = 0; i < mPccParams.slsParams.level; i++) {
            key = "coeff";
            sprintf(temp, "%d", i);
            key.append(temp);
            result = xml_handle.getXmlValue(key,p,sizeof(p));
            if(result >= 0) {
                if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                    for (int j = 0; j < 9; j++) {
                        mPccParams.slsParams.coeff[i][j] = atof(tokens[j]);
                        DF_LOGV("Sunlight Screen coeff %d %d %f ",i, j, atof(tokens[j]));
                    }
                }
            }
        }
        key = "threshold";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                for (int i = 0;i < mPccParams.slsParams.level - 1; i++) {
                    mPccParams.slsParams.luxThreshold[i] = atoi(tokens[i]);
                    DF_LOGV("Sunlight Screen threshold[%d]=%d ",i,atoi(tokens[i]));
                }
            }
        }
        for (int j = 0; j < 3; j++) {
            key = "paperCoeff";
            sprintf(temp, "%d", j);
            key.append(temp);
            result = xml_handle.getXmlValue(key,p,sizeof(p));
            if(result >= 0) {
                if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                    for (int i = 0; i < 9; i++) {
                        mPccParams.PaperParams.coeff[j][i] = atof(tokens[i]);
                        DF_LOGE("mPccParams.PaperParams.coeff[%d][%d]=%f ",j,i, mPccParams.PaperParams.coeff[j][i]);
                    }
                }
            }
        }
        key = "checkNum";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mFFCheckNum = atoi(p);
            DF_LOGV("FF check num:%d", mFFCheckNum);
        }
        for (int j = 0; j < mFFCheckNum; j++) {
            key = "checkData";
            sprintf(temp, "%d", j);
            key.append(temp);
            result = xml_handle.getXmlValue(key,p,sizeof(p));
            if(result >= 0) {
                if (!xml_handle.ParseLine(p, " \n", tokens, max_count, &token_count)) {
                    for (int i = 0; i < 9; i++) {
                        mFFData.push_back(atoi(tokens[i]));
                        DF_LOGV("mFFData %d %d value %d",j, i, atoi(tokens[i]));
                    }
                }
            }
        }
        key = "ExpertEnable";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mExpertEnable = atoi(p);
            DF_LOGV("mExpertEnable: %d",atoi(p));
        }else{
            DF_LOGV("parse %s failed", key.c_str());
            mExpertEnable = 1;
        }
        key = "DefaultGamut";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mDefaultGamut = atoi(p);
            DF_LOGV("mDefaultGamut: %d",atoi(p));
        }else{
            DF_LOGV("parse %s failed", key.c_str());
            mDefaultGamut = 1;
        }
        key = "DRELux";
        result = xml_handle.getXmlValue(key,p,sizeof(p));
        if(result >= 0) {
            mDRELux = atoi(p);
            DF_LOGV("mDRELux: %d",atoi(p));
        }else{
            DF_LOGV("parse %s failed", key.c_str());
            mDRELux = 0;
        }
    }

    setParams(SET_PCC_MODE,(void *)&mPccParams);
    sParams.function_id = DISPLAY_CAL_WP;
    err |= setDisplayEffect(SET_PCC_MODE, &sParams);
    if (mReadMaxBrightnessEnable) {
        DisplayUtil::readWPMaxBrightness((long*) &maxBrightness);
        realBrightness = mStartBrightness + (long)maxBrightness * mBrightnessStep;
    } else {
        realBrightness = mTypBrightness;
    }
    snprintf(property_value, 4, "%d", realBrightness);
    property_set(PERSIST_VENDOR_MAX_BRIGHTNESS, property_value);

    DisplayUtil::readMaxBrightness((long*) &maxBLLevel);
    return err;
}

void DisplayEffectBase::setAIDimParam(int step)
{
#ifdef HAS_QDCM
    sp<IMiStcService> imistcservice;
    android::Parcel data;
    int ret = 0;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(IMiStcService::PARAM_AI_DIM_STEP);
    data.writeInt32(mDisplayId);
    data.writeInt32(step);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_PARAM, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }
#endif
}
}
