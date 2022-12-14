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

#ifndef _PARAM_MANAGER_H_
#define _PARAM_MANAGER_H_

#include <string>
#include <map>
#include <unistd.h>
#include "tinyxml2.h"

using std::map;
using std::string;
using std::make_pair;
using std::pair;
using namespace tinyxml2;

enum EXPERT_NODE {
    EXPERT_NODE_NAME,
    EXPERT_NODE_ENABLE,
    EXPERT_NODE_GAMUT,
    EXPERT_NODE_R,
    EXPERT_NODE_G,
    EXPERT_NODE_B,
    EXPERT_NODE_H,
    EXPERT_NODE_S,
    EXPERT_NODE_V,
    EXPERT_NODE_CONT,
    EXPERT_NODE_GAMMA,
    EXPERT_NODE_MAX,
};

namespace android {

class ParamManager {
public:
    ParamManager();
    ~ParamManager();
    int getParam(const string Node, const string Param);
    bool setParam(const string Node, const string Param, int value);
    bool BackupXMLFile(int status);
    bool syncParam();

    const string ExpertNode[EXPERT_NODE_MAX] =
            {"Expert", "Enable", "Gamut", "R", "G", "B", "H", "S", "V", "Contrast", "Gamma"};

private:
    bool Init();
    bool CreateXMLFile(const map<string, string> &T_Map);

    const string ParamFileName = "/data/vendor/display/DF_Param.xml";
    const string BakParamFileName = "/data/vendor/display/DF_Param_Bak.xml";
    bool mInit;
};

};
#endif //_PARAM_MANAGER_H_

