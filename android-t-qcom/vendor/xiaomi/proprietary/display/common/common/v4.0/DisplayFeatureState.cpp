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

#include <utils/Log.h>
#include <cutils/properties.h>
#include "DisplayFeatureState.h"
#include "OutputFactory.h"
#include "display_effects.h"

#define LOG_TAG "DisplayFeatureHal"
#define LOG_NDEBUG 0

#define PROPERTY_PRODUCT_MODEL "ro.product.model"


namespace android {

DisplayFeatureState::DisplayFeatureState(unsigned int displayId)
    :mOutPutMethod(NULL)
    , mDisplayId(displayId)
{
}

DisplayFeatureState::~DisplayFeatureState()
{
}

void DisplayFeatureState::init(bool support)
{
    mSupport = support;
    memset(&mParam , 0, sizeof(OutputParam));
    getMethodByProduct();
}

int DisplayFeatureState::getMethodByProduct()
{
    char propertyValue[PROPERTY_VALUE_MAX];
    OutputFactory &factory = OutputFactory::getInstance();

    if (property_get(PROPERTY_PRODUCT_MODEL, propertyValue, NULL) < 0) {
        return -1;
    }

    mOutPutMethod = factory.createProduct(mDisplayId, mFeatueId, propertyValue);

    if(mOutPutMethod == NULL)
        return -1;

    return 0;
}

bool DisplayFeatureState::getIsEnableByValue(int value)
{
    return value == 0 ? false : true;
}

int DisplayFeatureState::output()
{
    int ret = -1;
    if (mOutPutMethod != NULL)
        return mOutPutMethod->output(mParam);
    return ret;
}

int DisplayFeatureState::isSupport()
{
    return mSupport;
}

void DisplayFeatureState::setValue(struct OutputParam *pSOutputParams)
{
    if (pSOutputParams) {
        memcpy(&mParam, pSOutputParams, sizeof(OutputParam));
    }
}

void DisplayFeatureState::dump(std::string& result)
{
    if (mOutPutMethod != NULL)
        mOutPutMethod->dump(result);
}

void DisplayFeatureState::setParam(void* params)
{
    if (mOutPutMethod != NULL && params != NULL)
        mOutPutMethod->setParam(params);
}
AdMode::AdMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_AD;
    ALOGD("%s %d", __func__, mFeatueId);
}
AdMode::~AdMode()
{
    ALOGD("%s", __func__);

}

SviMode::SviMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_SVI;
    ALOGD("%s %d", __func__, mFeatueId);
}

SviMode::~SviMode()
{
    ALOGD("%s", __func__);
}

PanelMode::PanelMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_PANEL_MODE;
    ALOGD("%s %d", __func__, mFeatueId);
}

PanelMode::~PanelMode()
{
    ALOGD("%s", __func__);
}

ColorMode::ColorMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_COLOR_MODE;
    ALOGD("%s %d", __func__, mFeatueId);
}

ColorMode::~ColorMode()
{
    ALOGD("%s", __func__);
}

PccMode::PccMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_PCC_MODE;
    ALOGD("%s %d", __func__, mFeatueId);
}

PccMode::~PccMode()
{
    ALOGD("%s", __func__);
}

DSPPMode::DSPPMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_DSPP_MODE;
    ALOGD("%s %d", __func__, mFeatueId);
}

DSPPMode::~DSPPMode()
{
    ALOGD("%s", __func__);
}

MTKMode::MTKMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_MTK_MODE;
    ALOGD("%s %d", __func__, mFeatueId);
}

MTKMode::~MTKMode()
{
    ALOGD("%s", __func__);
}

LtmMode::LtmMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_LTM;
    ALOGD("%s %d", __func__, mFeatueId);
}
LtmMode::~LtmMode()
{
    ALOGD("%s", __func__);

}

DumpMode::DumpMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_BUF_DUMP;
    ALOGD("%s %d", __func__, mFeatueId);
}
DumpMode::~DumpMode()
{
    ALOGD("%s", __func__);

}

QDCMMode::QDCMMode(unsigned int displayId)
    :DisplayFeatureState(displayId)
{
    mFeatueId = SET_QDCM_MODE;
    ALOGD("%s %d", __func__, mFeatueId);
}
QDCMMode::~QDCMMode()
{
    ALOGD("%s", __func__);

}
}//namespace android


