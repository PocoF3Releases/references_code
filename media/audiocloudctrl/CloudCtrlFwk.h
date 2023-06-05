#ifndef __CLOUDCONTROLFWK_H_
#define __CLOUDCONTROLFWK_H_
#include "CloudCtrlBase.h"
namespace AudioCloudCtrl{
class CloudCtrlFwk : public CloudCtrlBase{
public:
    static bool isSupportCloudCtrl;
    static std::map<std::string, std::set<std::string>> data_fwk_map;
    static bool isKvPairsExist(const char* key, const char* value); //select the value of the key in database exist or not.
    static const char* getValues(const char* key);
    static int delKVPairs(const char* key, const char* values); //use to delete values of the key from cloud_ctrl_data_xml.
    static int newKVPairs(const char* key, const char* values); //use to add new key and its values to cloud_ctrl_data_xml.
    static int addKVPairs(const char* key, const char* values); //use to add values of the key to cloud_ctrl_data_xml.
    static int CloudCtrlParser(std::string cloud_ctrl_data);//get parser xml from hal to data_fwk_map
};
}
#endif