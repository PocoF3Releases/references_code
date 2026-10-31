#define LOG_TAG "AudioCloudCtrl"
#define LOG_NDDEBUG 0
#include <log/log.h>
#include <errno.h>
#include <sstream>
#include "CloudCtrlFwk.h"
namespace AudioCloudCtrl{
std::shared_mutex update_fwk_map_mutex; //global mutex when getting share, updating lock.
std::map<std::string, std::set<std::string>> CloudCtrlFwk::data_fwk_map = {};
bool CloudCtrlFwk::isSupportCloudCtrl = true;
bool CloudCtrlFwk::isKvPairsExist(const char* key, const char* value)
{
    std::shared_lock lock(update_fwk_map_mutex);
    if(data_fwk_map.find(key) != data_fwk_map.end())
    {
        if((data_fwk_map.find(key)->second).find(value) != (data_fwk_map.find(key)->second).end())
            return true;
    }
    return false;
}
const char* CloudCtrlFwk::getValues(const char* key)
{
    std::shared_lock lock(update_fwk_map_mutex);
    std::string values = "";
    if(data_fwk_map.find(key) != data_fwk_map.end())
    {
        if(!(data_fwk_map.find(key)->second).empty()){
            for(std::string value : data_fwk_map.find(key)->second){
                values.append(value.c_str());
                values.append(" ");
            }
        }
    }
    const char* res = values.c_str();
    return res;
}
int CloudCtrlFwk::addKVPairs(const char* key, const char* values)
{
    if(values == NULL)
        return -1;
    std::stringstream ss(values);
    std::string word;
    std::lock_guard<std::shared_mutex> lock(update_fwk_map_mutex); //lock to avoid multiThread modify
    if(data_fwk_map.find(key) == data_fwk_map.end()){
        ALOGD("there is no key %s",key);
        return -1;
    }else {
        if((data_fwk_map.find(key)->second).find(values) == (data_fwk_map.find(key)->second).end()){
            ALOGD("handle values=%s", values);
            while (ss >> word) {
                data_fwk_map.find(key)->second.insert(word);
                ALOGD("add appname %s to key %s", word.c_str(), key);
            }
        }
    }

    return 1;
}
int CloudCtrlFwk::newKVPairs(const char* key, const char* values)
{
    if(values == NULL)
        return -1;
    std::set<std::string> valueset;
    std::stringstream ss(values);
    std::string word;
    std::lock_guard<std::shared_mutex> lock(update_fwk_map_mutex); //lock to avoid multiThread modify
    if(data_fwk_map.find(key) == data_fwk_map.end()){
        ALOGD("add new key %s to xml",key);
        while (ss >> word) {
            valueset.insert(word);
            ALOGD("add appname %s to key %s", word.c_str(), key);
        }
        data_fwk_map.insert(std::make_pair(key, valueset));
        return 1;
    }else {
        if ((data_fwk_map.find(key)->second).find(values) == (data_fwk_map.find(key)->second).end()){
            ALOGD("handle values=%s", values);
            while (ss >> word) {
                data_fwk_map.find(key)->second.insert(word);
                ALOGD("add appname %s to key %s", word.c_str(), key);
            }
        }
    }
    return 1;
}
int CloudCtrlFwk::delKVPairs(const char* key, const char* values)
{
    if(values == NULL)
        return -1;
    std::stringstream ss(values);
    std::string word;
    std::lock_guard<std::shared_mutex> lock(update_fwk_map_mutex); //lock to avoid multiThread modify
    if(data_fwk_map.find(key) == data_fwk_map.end()){
        ALOGD("there is no key %s in xml",key);
        return -1;
    }
    while (ss >> word) {
        if((data_fwk_map.find(key)->second).find(word) != (data_fwk_map.find(key)->second).end()){
            data_fwk_map.find(key)->second.erase(word);
            ALOGD("delete appname %s from key %s", word.c_str(), key);
        }
    }

    return 1;
}
int CloudCtrlFwk::CloudCtrlParser(std::string cloud_ctrl_data)
{
    std::lock_guard<std::shared_mutex> lock(update_fwk_map_mutex); //lock to avoid multiThread modify
    data_fwk_map.erase(data_fwk_map.begin(),data_fwk_map.end());
    if(cloud_ctrl_data.empty()){
        ALOGE("get data from xml failed!");
        return -1;
    }
    //"[tag1]:v1 v11 [tag2]:v2 v22 [tag3]:v3 [tag4]:v4 v4 v444 v4 [tag5]:[tag6]:"
    std::string str(cloud_ctrl_data.c_str());
    str.erase(0,(str.find("=")+1));  //delete header info in param
    int begin=str.find("[");int end=str.find("]");
    std::set<std::string> values;
    std::string key;
    while(begin<end) {  //parser each tag of xml
        key = str.substr(begin+1,end-begin-1);
        str.erase(begin,key.size()+3);
        int start=0;
        int div=str.find(" ");
        if(str.empty()){
            data_fwk_map.insert(std::make_pair(key,values));
            break;
        }
        while(start<div&&str.at(start)!='['){//parser each value of tag
            values.insert(str.substr(start,div-start));
            //ALOGD("value %s insert to key %s",str.substr(start,div-start).c_str(),key.c_str());
            str.erase(start,div-start+1);
            div=str.find(" ");
            if(str.empty()){
                break;
                }
            }
        data_fwk_map.insert(std::make_pair(key,values));//insert data_fwk_map
        values.erase(values.begin(),values.end()); //clean values temp
        begin=str.find("[");//begin of next tag
        end=str.find("]");  //end of next tag
        if(begin==-1)
            break;
        else
            str.erase(0,begin);
        }
    ALOGD(" Framework Parser Done");
    for(std::map<std::string, std::set<std::string>>::iterator it=data_fwk_map.begin();it!=data_fwk_map.end();it++){
        ALOGD("KEY: %s ",it->first.c_str());
        for(std::set<std::string>::iterator ite=it->second.begin();ite!=it->second.end();ite++)
            ALOGD("VALUE: %s ",ite->c_str());
    }
    return 1;
}
}