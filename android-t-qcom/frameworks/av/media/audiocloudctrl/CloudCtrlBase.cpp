#define LOG_TAG "AudioCloudCtrl"
#define LOG_NDDEBUG 0
#include <log/log.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <tinyxml2.h>
#include <thread>
#include <sstream>
#include "CloudCtrlBase.h"
namespace AudioCloudCtrl{
std::shared_mutex update_xml_map_mutex; //global mutex when getting share, updating lock.
std::shared_ptr<CloudCtrlBase> CloudCtrlBase::mcloudctrl = nullptr;
std::map<std::string, std::set<std::string>> CloudCtrlBase::data_base_map = {};
std::map<std::string, int> CloudCtrlBase::data_property_int = {};
std::map<std::string, std::string> CloudCtrlBase::data_property_bool = {};
const char* CloudCtrlBase::mcloud_ctrl_data_path = CLOUD_CONTROL_DATA_PATH;
std::shared_ptr<CloudCtrlBase> CloudCtrlBase::getInstance()
{
    if (!mcloudctrl) {
        //ALOGD("NEW CloudCtrlBase!");
        mcloudctrl = std::shared_ptr<CloudCtrlBase> (new CloudCtrlBase());
    }
    return mcloudctrl;
}
std::string CloudCtrlBase::audiocloudctrl_fwk_init()
{
    std::shared_lock lock(update_xml_map_mutex);
    std::string database;
    std::string left="[";
    std::string right="]:";
    for(std::map<std::string, std::set<std::string>>::iterator it=data_base_map.begin();it!=data_base_map.end();it++){
        database.append(left);
        database.append(it->first);
        database.append(right);
        for(std::set<std::string>::iterator ite=it->second.begin();ite!=it->second.end();ite++){
            database.append(*ite);
            database.append(" ");
        }
    }
    return database;
}
bool CloudCtrlBase::isWhlitelistEmpty()
{
    std::shared_lock lock(update_xml_map_mutex);
    return data_base_map.empty();
}
bool CloudCtrlBase::isKvPairsExist(const char* key, const char* value)
{
    std::shared_lock lock(update_xml_map_mutex);
    if(data_base_map.find(key) != data_base_map.end())
    {
        if((data_base_map.find(key)->second).find(value) != (data_base_map.find(key)->second).end())
            return true;
    }
    return false;
}
int CloudCtrlBase::get_int_property(const char* value)
{
    std::shared_lock lock(update_xml_map_mutex);
    if(data_property_int.find(value) != data_property_int.end()){
        return data_property_int.find(value)->second;
    }
    return 0;
}
bool CloudCtrlBase::get_bool_property(const char* value)
{
    std::shared_lock lock(update_xml_map_mutex);
    if(data_property_bool.find(value) != data_property_bool.end()){
        if(data_property_bool.find(value)->second == "true"){
            return true;
        }
    }
    return false;
}
int CloudCtrlBase::newKVPairs(const char* key, const char* values)
{
    if(!key || !values)
        return -1;
    //TODO verify key and values is legal.
    std:: string str = values;
    str += " +";
    return UpdateCloudCtrlXml(mcloud_ctrl_data_path, key, str.c_str(), true);
}
int CloudCtrlBase::addKVPairs(const char* key, const char* values)
{
    if(!key || !values)
        return -1;
    std:: string str = values;
    str += " +";
    return UpdateCloudCtrlXml(mcloud_ctrl_data_path, key, str.c_str());
}
int CloudCtrlBase::delKVPairs(const char* key, const char* values)
{
    if(!key || !values)
        return -1;
    std:: string str = values;
    str += " -";
    return UpdateCloudCtrlXml(mcloud_ctrl_data_path, key, str.c_str());
}
bool CloudCtrlBase::handle_values(const char* values, std::stack<std::string>& stk)
{
    if(values == NULL)
        return false;
    std::stringstream ss(values);
    std::string word;
    ALOGD("handle values=%s", values);
    while (ss >> word) {
        stk.push(word);
    }
    if(!stk.empty())
        return true;
    return false;
}
bool add_kv_to_xml(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* currentele, const char* key, const char* value)
{
     if(!strncmp(key, "property_int_list", 17)){
        char ptr_str[64];
        strncpy(ptr_str, value, sizeof(char)*(strlen(value)+1));
        char* paras;
        char* str = strtok_r(ptr_str, ":", &paras);
        if(CloudCtrlBase::data_property_bool.find(str) == CloudCtrlBase::data_property_bool.end()){
            tinyxml2::XMLElement* userNode = doc.NewElement(str);
            userNode->SetText(paras);
            currentele->InsertEndChild(userNode);
            ALOGD("add property_int_list key_%s value_%s successfully!", str, paras);
            return true;
        } else{
            for(tinyxml2::XMLElement* modnode = currentele->FirstChildElement(); modnode != NULL; modnode = modnode->NextSiblingElement()){
                if(!strncmp(modnode->Name(), str, sizeof(char)*(strlen(str)+1))){
                    modnode->SetText(paras);
                    ALOGD("mod property_int_list key_%s value_%s successfully!", str, paras);
                }
            }
            return true;
        }
        return false;
    }
    if(!strncmp(key, "property_bool_list", 18)){
        char ptr_str[64];
        strncpy(ptr_str, value, sizeof(char)*(strlen(value)+1));
        char* paras;
        char* str = strtok_r(ptr_str, ":", &paras);
        if(CloudCtrlBase::data_property_bool.find(str) == CloudCtrlBase::data_property_bool.end()){
            tinyxml2::XMLElement* userNode = doc.NewElement(str);
            userNode->SetText(paras);
            currentele->InsertEndChild(userNode);
            ALOGD("add property_bool_list key_%s value_%s successfully!", str, paras);
            return true;
        } else{
            for(tinyxml2::XMLElement* modnode = currentele->FirstChildElement(); modnode != NULL; modnode = modnode->NextSiblingElement()){
                if(!strncmp(modnode->Name(), str, sizeof(char)*(strlen(str)+1))){
                    modnode->SetText(paras);
                    ALOGD("mod property_int_list key_%s value_%s successfully!", str, paras);
                }
            }
            return true;
        }
        return false;
    }
    if((CloudCtrlBase::data_base_map.find(key)->second).find(value) == (CloudCtrlBase::data_base_map.find(key)->second).end()){
        tinyxml2::XMLElement* userNode = doc.NewElement(value);
        currentele->InsertEndChild(userNode);
        ALOGD("add appname %s to key %s", value, key);
        return true;
    }
    return false;
}
bool delete_kv_from_xml(tinyxml2::XMLElement* currentele, const char* key, const char* value)
{
    if(!strncmp(key, "property_int_list", 18)){
        char ptr_str[64];
        strncpy(ptr_str, value, sizeof(char)*(strlen(value)+1));
        char* paras;
        char* str = strtok_r(ptr_str, ":", &paras);
        if(CloudCtrlBase::data_property_int.find(str) != CloudCtrlBase::data_property_int.end()){
            for(tinyxml2::XMLElement* delnode = currentele->FirstChildElement(); delnode != NULL; delnode = delnode->NextSiblingElement()){
                if(!strncmp(delnode->Name(), str, sizeof(char)*(strlen(str)+1))){
                    currentele->DeleteChild(delnode);
                    ALOGD("delete property_int_list key_%s value_%s successfully!", str, paras);
                    return true;
                }
            }
        }
        ALOGD("delete property_int_list key_%s not found!", str);
        return false;
    }
    if(!strncmp(key, "property_bool_list", 18)){
        char ptr_str[64];
        strncpy(ptr_str, value, sizeof(char)*(strlen(value)+1));
        char* paras;
        char* str = strtok_r(ptr_str, ":", &paras);
        if(CloudCtrlBase::data_property_bool.find(str) != CloudCtrlBase::data_property_bool.end()){
            for(tinyxml2::XMLElement* delnode = currentele->FirstChildElement(); delnode != NULL; delnode = delnode->NextSiblingElement()){
                if(!strncmp(delnode->Name(), str, sizeof(char)*(strlen(str)+1))){
                    currentele->DeleteChild(delnode);
                    ALOGD("delete property_bool_list key_%s value_%s successfully!", str, paras);
                    return true;
                }
            }
        }
        ALOGD("delete property_bool_list key_%s not found!", str);
        return false;
    }
    for(tinyxml2::XMLElement* delnode = currentele->FirstChildElement(); delnode != NULL; delnode = delnode->NextSiblingElement()){
        if(!strncmp(delnode->Name(), value, sizeof(char)*(strlen(value)+1))){
            currentele->DeleteChild(delnode);
            ALOGD("delete appname %s form key %s", value, key);
            return true;
        }
    }
    return false;
}
int CloudCtrlBase::UpdateCloudCtrlXml(const char* xmlFile, const char* key, const char *values, bool isNewKey/*default false*/)
{
    if(!xmlFile || !key || !values)
        return -1;
    int ret = 0;
    std::lock_guard<std::shared_mutex> lock(update_xml_map_mutex); //lock to avoid multiThread modify
    tinyxml2::XMLDocument doc;
    ret = doc.LoadFile(xmlFile);
    if (tinyxml2::XML_SUCCESS != ret)
    {
        ALOGE("Failed to Update xmlLoadFile error %d", ret);
        return ret;
    }
    //TODO: add version control
    tinyxml2::XMLElement* root = doc.RootElement();
    //TODO: find isNewkey ornot in xml
    if(isNewKey && (data_base_map.find(key) == data_base_map.end())){
        tinyxml2::XMLElement* userNode = doc.NewElement(key);
        root->InsertEndChild(userNode);
        std::set<std::string> set0;
        data_base_map.insert(std::make_pair(key, set0));
    }
    //TODO: add all and delete all
    for(tinyxml2::XMLElement* currentele = root->FirstChildElement(); currentele != NULL; currentele = currentele->NextSiblingElement()){
        if(!strncmp(currentele->Name(), key, sizeof(char)*(strlen(key)+1))){
            std::stack<std::string> stk;
            if(handle_values(values, stk)){
                if(stk.empty() || !(stk.top().c_str())){
                    ALOGE("Failed to UpdateWhitelistXml, no valid values");
                    break;
                }
                if(!strncmp("+", stk.top().c_str(), strlen("+"))){
                    stk.pop();
                    while(!stk.empty()){
                        add_kv_to_xml(doc, currentele, key, stk.top().c_str());
                        stk.pop();
                    }
                    break;
                }else if(!strncmp("-", stk.top().c_str(), strlen("-"))){
                    stk.pop();
                    while(!stk.empty()){
                        delete_kv_from_xml(currentele, key, stk.top().c_str());
                        stk.pop();
                    }
                    break;
                }else{
                    ALOGE("Failed to UpdateWhitelistXml, no add or delete symbol");
                    break;
                }
            }
        }
    }
    ret = doc.SaveFile(xmlFile);
    if (tinyxml2::XML_SUCCESS != ret)
    {
        ALOGE("Failed to xmlSaveFile error %d", ret);
        return -1;
    }
    ret = CloudCtrlXmlParser(xmlFile);
    return ret;
}
int CloudCtrlBase::CloudCtrlXmlParser(const char* xmlFile)
{
    int ret = 0;
    tinyxml2::XMLDocument doc;
    data_base_map.erase(data_base_map.begin(),data_base_map.end());
    ret = doc.LoadFile(xmlFile);
    if (tinyxml2::XML_SUCCESS != ret)
    {
        ALOGE("Failed to Parser xmlLoadFile error %d", ret);
        return -1;
    }
    ALOGD("XML parsing started - file name %s", xmlFile);
    //TODO version control
    tinyxml2::XMLElement* root = doc.RootElement();
    for(tinyxml2::XMLElement* currentkey = root->FirstChildElement(); currentkey != NULL; currentkey = currentkey->NextSiblingElement()){
        std::set<std::string> value_set;
        for(tinyxml2::XMLElement* currentvalue = currentkey->FirstChildElement(); currentvalue != NULL; currentvalue = currentvalue->NextSiblingElement()){
            if(!strncmp(currentkey->Name(), "property_int_list", 17)){
                data_property_int.insert(std::make_pair(currentvalue->Name(), std::stoi(currentvalue->GetText())));
                 ALOGV("key_set=%s, value_set=%s", currentkey->Name(), currentvalue->GetText());
            } else if(!strncmp(currentkey->Name(), "property_bool_list", 18)){
                data_property_bool.insert(std::make_pair(currentvalue->Name(),currentvalue->GetText()));
                ALOGV("key_set=%s, value_set=%s", currentkey->Name(), currentvalue->GetText());
            }else {
                value_set.insert(currentvalue->Name());
                ALOGV("key_set=%s, value_set=%s", currentkey->Name(), value_set.find(currentvalue->Name())->c_str());
            }
        }
        data_base_map.insert(std::make_pair(currentkey->Name(), value_set));
    }
    if (data_base_map.empty())
    {
        ret = -1;
        ALOGE("Failed to Parser Xml or Xml is empty %d", ret);
        return ret;
    }
    return tinyxml2::XML_SUCCESS;
}
void CloudCtrlBase::initCloudCtrl()
{
    if(!access(mcloud_ctrl_data_path, F_OK)) {
        CloudCtrlXmlParser(mcloud_ctrl_data_path);
    }else{
        if(copy_xml_file(CLOUD_CONTROL_ETC_PATH, mcloud_ctrl_data_path) == -1)
            ALOGE("init CloudControl etc fail");
        else
            ALOGD("init CloudControl etc");
    }
}
int CloudCtrlBase::copy_xml_file(const char *sfn, const char *ofn)
{
    struct stat statbf;
    int sfd = open(sfn, O_RDONLY);
    if(sfd == -1){
        ALOGE("open file error on %d\n", errno);
        return sfd;
    }
    if(fstat(sfd, &statbf) == -1){
        close(sfd);
        return -1;
    }
    int ofd = open(ofn, O_WRONLY|O_CREAT, statbf.st_mode);
    if(ofd == -1){
        ALOGE("open file error on %d\n", errno);
        close(sfd);
        return ofd;
    }
    int ret = sendfile(ofd, sfd, 0, statbf.st_size);
    close(sfd);
    close(ofd);
    if(ret != -1)
        return CloudCtrlXmlParser(ofn);
    return ret;
}
}