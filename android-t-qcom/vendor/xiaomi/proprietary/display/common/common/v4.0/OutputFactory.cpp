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
#include "OutputFactory.h"
#include "OutputMethod.h"
#include "display_effects.h"

#define LOG_TAG "DisplayFeatureHal"
#define LOG_NDEBUG 0

namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(OutputFactory);

OutputFactory::OutputFactory()
    : Singleton<OutputFactory>()
{
    ALOGV("OutputFactory created");
}

OutputFactory::~OutputFactory()
{
    ALOGV("OutputFactory destroyed");
}

OutputMethod* OutputFactory::createProduct(unsigned int displayId, int featureId, char* model)
{
    OutputMethod* product = NULL;

    switch (featureId) {
        case SET_AD: {
            product = new AdModeImpl(displayId);
        } break;
        case SET_PANEL_MODE: {
            product = new PanelModeImpl(displayId);
        } break;
        case SET_COLOR_MODE: {
            product = new ColorModeImpl(displayId);
        } break;
        case SET_PCC_MODE: {
            product = new PccModeImpl(displayId);
        } break;
        case SET_DSPP_MODE: {
            product = new DSPPModeImpl(displayId);
        } break;
        case SET_MTK_MODE: {
            product = new MTKModeImpl(displayId);
        } break;
        case SET_LTM: {
            product = new LtmModeImpl(displayId);
        } break;
        case SET_BUF_DUMP: {
            product = new BufDumpImpl(displayId);
        } break;
        default:
            break;
    }
    return product;
}
}
