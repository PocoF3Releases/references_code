// FIXME: your file license if you have one

#include "Dtool.h"
#include <log/log.h>
#include <android/log.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <cutils/properties.h>

namespace vendor::xiaomi::hardware::dtool::implementation {

using namespace std;

string path="/data/vendor/fpdump";
vector<string> nameList;
string pass="fp_log.txt";

void getAllFiles(string path, vector<string>& files)
{
    DIR *dir;
    struct dirent *ptr;
    if((dir=opendir(path.c_str()))==NULL)
    {
        perror("DtoolH: Open dir error...");
        exit(1);
    }
    while((ptr=readdir(dir))!=NULL)
    {
        if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
            continue;
        else if(ptr->d_type==8)//file
        {
            if(pass.compare(ptr->d_name))
                files.push_back(path+"/"+ptr->d_name);
        }
        else if(ptr->d_type==10)//link file
            continue;
        else if(ptr->d_type==4)
            getAllFiles(path+"/"+ptr->d_name,files);
    }
    closedir(dir);
}

// Methods from ::vendor::xiaomi::hardware::dtool::V1_0::IDtool follow.
Return<int32_t> Dtool::getNum() {
    // TODO implement
    nameList.clear();
    getAllFiles(path, nameList);
    int fi=nameList.size();

    int32_t te=fi;
    ALOGD("DtoolH: num of files is %d",te);
    return te;
}

Return<void> Dtool::getName(int32_t num, getName_cb _hidl_cb) {
    // TODO implement
    _hidl_cb(nameList[num]);
    return Void();
}

Return<void> Dtool::getFile(const hidl_string& name, int32_t num, getFile_cb _hidl_cb) {
    // TODO implement
    ifstream fin(name);
    vector<int8_t> buf;
    int32_t length = 1000000;
    int32_t starting_point = num*1000000;
    
    char tmp;
    if(!fin)
        perror("DtoolH: file is null! ");

    fin.seekg(starting_point, ios_base::beg);
    while(!fin.eof() && length!=0)
    {
        fin.get(tmp);
        buf.push_back((int8_t)tmp);
        length--;
    }
    fin.close();
    _hidl_cb(buf);
    return Void();
}

Return<int32_t> Dtool::getLength(const hidl_string& name) {
    // TODO implement
    ifstream fin(name);
    if(!fin)
        perror("DtoolH: file is null! ");

    fin.seekg(0, ios_base::end);
    int nFileLen = fin.tellg();
    ALOGD("DtoolH: nFileLen is %d",nFileLen);

    return nFileLen;
}

Return<int32_t> Dtool::setdumpprop(int32_t num) {
    // TODO implement
    int err = 0;
    if(num == 1)
        err = property_set("persist.vendor.sys.fp.dump_data", "1");
    else if(num == 0)
        err = property_set("persist.vendor.sys.fp.dump_data", "0");

    if(err!=0)
    {
        ALOGD("DtoolH: setprop %s is failed!", "persist.vendor.sys.fp.dump_data");
        goto exit;
    }

    ALOGD("DtoolH: setprop %s %d", "persist.vendor.sys.fp.dump_data", num);

exit:
    return err;
}

Return<int32_t> Dtool::getdumpprop() {
    // TODO implement
    char prop[PROPERTY_VALUE_MAX] = {'\0'};
    int err = 0;
    int result = -1;
    ALOGD("DtoolH: property_get enter!");
    err = property_get("persist.vendor.sys.fp.dump_data", prop, "0");
    ALOGD("DtoolH: property_get end!");

    if(err>0 && strlen(prop)>0 && atoi(prop)==0)
        result = 0;
    else if(err>0 && strlen(prop)>0 && atoi(prop)==1)
        result = 1;
    else {
        ALOGD("DtoolH: getprop %s is failed!","persist.vendor.sys.fp.dump_data");
        goto exit;
    }

    ALOGD("DtoolH: getprop %s = %d","persist.vendor.sys.fp.dump_data", result);

exit:
    return result;
}

Return<int32_t> Dtool::deleteFile(const hidl_string& name) {
    // TODO implement
    int result = -1;
    result = remove(name.c_str());
    if(result!=0) {
        return -1;
    }

    return 0;
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//IDtool* HIDL_FETCH_IDtool(const char* /* name */) {
    //return new Dtool();
//}
//
}  // namespace vendor::xiaomi::hardware::dtool::implementation
