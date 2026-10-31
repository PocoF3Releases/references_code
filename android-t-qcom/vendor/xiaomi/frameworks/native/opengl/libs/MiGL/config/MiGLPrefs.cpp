#include <utils/Errors.h>
#include "MiGLPrefs.h"
#include<cstring>

using namespace std;
namespace com {
namespace xiaomi {
namespace joyose {

#define TEXTURE_SIZE 1
#define SHADING_RATE 2
#define TEXTURE_PARAM 3
#define YUANSHEN_SCENE_RECOGNIZE 4

// important!!
int MiGLPrefs::readFromJoyose(std::string cmds) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(cmds, root);
    params = root["configs"];
    queryParams(TEXTURE_SIZE);
    queryParams(SHADING_RATE);
    queryParams(TEXTURE_PARAM);
    queryParams(YUANSHEN_SCENE_RECOGNIZE);
    return 0;
}
vector<string> split(const string &s, const string &seperator){
    vector<string> result;
    typedef string::size_type string_size;
    string_size i = 0;

    while(i != s.size()){
        int flag = 0;
        while(i != s.size() && flag == 0) {
            flag = 1;
            for(string_size x = 0; x < seperator.size(); ++x) {
                if(s[i] == seperator[x]) {
                    ++i;
                    flag = 0;
                    break;
                }
            }
        }

        flag = 0;
        string_size j = i;
        while(j != s.size() && flag == 0) {
            for(string_size x = 0; x < seperator.size(); ++x) {
                if(s[j] == seperator[x]) {
                    flag = 1;
                    break;
                }
            }
            if(flag == 0)
                ++j;
        }

        if(i != j) {
            result.push_back(s.substr(i, j-i));
            i = j;
        }
    }
  return result;
}

void MiGLPrefs::queryParams(int features) {
    switch(features) {
        case TEXTURE_SIZE:
            {
                string targetSize = params["tex_size"].asString();
                if (!targetSize.empty()) {
                    vector<string> v = split(targetSize,"x");
                    int width = stoi(v[0]);
                    int height = stoi(v[1]);
                    if((width > 0) && (height > 0)) {
                        scaleResolutionSize = true;
                        targetWidthSize = width;
                        targetHeightSize = height;
                    }
                }
            }
            break;
        case SHADING_RATE:
            {
                string shadingrate_params = params["shader_rate"].asString();
                if (!shadingrate_params.empty()) {
                    int shadingRate = stoi(shadingrate_params);
                    if(shadingRate > 0) {
                        modifyShadingRate = true;
                        targetShadingRate = shadingRate;
                    }
                }
            }
            break;
        case TEXTURE_PARAM:
            {
                string af_params = params["tex_param_anisotropy"].asString();
                if (!af_params.empty()) {
                    int AF_Value = stoi(af_params);
                    if(AF_Value > 0) {
                        modifyAF = true;
                        targetAFValue = AF_Value;
                    }
                }
            }
            break;
        case YUANSHEN_SCENE_RECOGNIZE:
            {
                string yssr_params = params["ys_scene_recognize"].asString();
                if (!yssr_params.empty()) {
                    enableYSSceneRecognize = true;
                }
            }
            break;
        default:
            ALOGE("Undefined pararm commends");
    }
}

}
}
}
