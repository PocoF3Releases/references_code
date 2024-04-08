#ifndef __CLOUDCTRLBASE_H_
#define __CLOUDCTRLBASE_H_

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <shared_mutex>
#include <mutex>
/* Audio Cloud Control Path*/
#define CLOUD_CONTROL_DATA_PATH "/data/vendor/audio/cloud_control_white_list.xml"
#define CLOUD_CONTROL_ETC_PATH "/vendor/etc/audio_cloud_control_white_list.xml"
namespace AudioCloudCtrl{

class CloudCtrlBase{
public:
    static std::shared_ptr<CloudCtrlBase> getInstance();
    static std::string audiocloudctrl_fwk_init();
    static bool isKvPairsExist(const char* key, const char* value); //select the value of the key in database exist or not.
    static bool isWhlitelistEmpty(); //check data_base_map empty or not.
    static int get_int_property(const char* value);
    static bool get_bool_property(const char* value);
    int addKVPairs(const char* key, const char* values); //use to add values of the key to cloud_ctrl_data_xml.
    int delKVPairs(const char* key, const char* values); //use to delete values of the key from cloud_ctrl_data_xml.
    int newKVPairs(const char* key, const char* values); //use to add new key and its values to cloud_ctrl_data_xml.
    static std::map<std::string, int > data_property_int;
    static std::map<std::string, std::string > data_property_bool;
    static std::map<std::string, std::set<std::string>> data_base_map;
protected:
    static std::shared_ptr<CloudCtrlBase> mcloudctrl;
    int UpdateCloudCtrlXml(const char* xmlFile, const char* key, const char *values, bool isNewKey = false); //inside handle function.
    int CloudCtrlXmlParser(const char* xmlFile); //parse cloud_ctrl_data_xml to database.
    bool handle_values(const char* values, std::stack<std::string>& stk); //inside handle function.
    int copy_xml_file(const char *sfn, const char *ofn); //inside handle function, copy xml from etc to data.
    CloudCtrlBase() {initCloudCtrl();}
    void initCloudCtrl();
    static const char* mcloud_ctrl_data_path;
};

}
#endif