#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>

#include <cutils/log.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <iostream>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <fstream>
#include <errno.h>
#include <utils/Errors.h>
#include <streambuf>
#include "sockets.h"

#include "f2fs_depend.h"

using namespace std;


#define MIN_FILE_SIZE 68988 //F2fs min processing size(byte)
#define ALIGNMENT_RULE 4096
#define MAX_BUF_LEN 64
#define DROP_CACHES_TRY 3
#define EACH_FILE_NUM 20
#define MIN_LBA_SIZE 128
#define MAX_SOCKET_SIZE 40960
#define MIN_STOP_TIME 360
#define APP_DEFRAG_TIME_LIMIT 3
#define FBO_INFO_PATH "/data/local/fbo/FboInfo.ini"

int fbo_state_flag = 0, Before_Defrag_LBA_NUM = 0, After_Defrag_LBA_NUM = 0;
string current_pkg = "", current_file = "", start_file = "";
time_t pre_stop_time = 0;

enum fbo_info_offset{
    APP_PKG = 0,
    FRAGMENT_NUM_BEFOR_DEFRAG,
    FRAGMENT_NUM_AFTER_DEFRAG,
    FBO_STOP_FILE,
    FBO_STOP_DATE,
};

extern "C"
{
    int dump_file_range(u32 nid, struct list_head *head, int force);
    int f2fs_init_get_file_zones(char *dev_path);
    void destory_file_zones(struct list_head *head);
    void f2fs_exit_get_file_zones();
    int defrag_file(u64 start, u64 len, char *file_path);
}

string ito_16str(u64 n){
	string str;
	stringstream ss;
	ss<<hex<<n;
	ss>>str;
    return str;
}


void get_time(time_t& cur_time, int& tm_day)
{
    struct tm *lt;

    cur_time =time(NULL);
    lt = localtime(&cur_time);
    tm_day = lt->tm_mday;
}

int get_all_file(const char *path, vector<file_info> &res /*  */)
{
    DIR *pDir;
    int ret = 0;
    struct dirent *ptr;
    struct stat st;
    if (!(pDir = opendir(path))){
        ret = stat(path, &st);
        if(ret){
            //Confirm whether there is no permission or a single file
            FBO_NATIVE_ERR_MSG("[FBO-NATIVE:stat] The directory cannot be opened!");
            return -1;
        }
        else if ((!S_ISDIR(st.st_mode))){
                if(st.st_size > MIN_FILE_SIZE){
                    file_info file_tmp(path, to_string(st.st_ino), to_string(st.st_size));
                    res.push_back(file_tmp);
                }
            return 0;
        }
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE:opendir] The directory cannot be opened!");
        return -1;
    }
    while ((ptr = readdir(pDir)) != 0){
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0){
            string temp = (string)path + "/" + (string)(ptr->d_name);
            stat(temp.c_str(), &st);
            if ((!S_ISDIR(st.st_mode))){
                if(st.st_size > MIN_FILE_SIZE){
                    file_info file_tmp(temp, to_string(st.st_ino), to_string(st.st_size));
                    res.push_back(file_tmp);
                }
            }else{
                get_all_file(temp.c_str(), res);
            }
        }
    }
    closedir(pDir);
    return 0;
}

string get_userdata_mapper()
{
    int ret;
    string dev_userdata = "";
    char buf[MAX_BUF_LEN];
    ret = readlink("/dev/block/mapper/userdata",buf, MAX_BUF_LEN - 1);
    if (ret < 0 || (ret >= MAX_BUF_LEN - 1)){
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] F2FS readlink fail.");
        return dev_userdata;
    }
    buf[ret] = '\0';
    dev_userdata = (string)buf;
    FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS userdata: %s.", buf);

    return dev_userdata;
}

int Set_FboInfo(int tm_day){
    ofstream write_data;
    string fbo_info;
    FBO_NATIVE_ERR_MSG("[FBO-NATIVE] tm_day is %d!.", tm_day);
    write_data.open(FBO_INFO_PATH, ios::out);
    if(!write_data.is_open()){
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] %s open fail!.", FBO_INFO_PATH);
        return -1;
    }
    // format  [app,fragment_num,fragment_num,stop_file,date]
    fbo_info = "[" + current_pkg + "," + to_string(Before_Defrag_LBA_NUM) + "," + to_string(After_Defrag_LBA_NUM) + "," + \
                current_file + "," + to_string(tm_day) + "]";
    
    FBO_NATIVE_DBG_MSG("[FBO-NATIVE] file info is %s." , const_cast<char *>(fbo_info.c_str()));
    write_data << fbo_info << endl;
    write_data.close();

    return 0;
}

int Get_FboInfo(vector<string>& res){
    ifstream read_data;
    string fbo_info;

    read_data.open(FBO_INFO_PATH, ios::in);
    if(!read_data.is_open()){
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] %s open fail!." , FBO_INFO_PATH);
        read_data.close();
        return -1;
    }

    fbo_info = string((istreambuf_iterator<char>(read_data)), istreambuf_iterator<char>());
    FBO_NATIVE_DBG_MSG("[FBO-NATIVE] fbo info is %s." , const_cast<char*>(fbo_info.c_str()));
    if(fbo_info.find("[") != string::npos){
        int pos = 0, pos_sta = 0;
        while((pos = fbo_info.find(",", pos+1)) != string::npos){
            string temp = fbo_info.substr(pos_sta, pos - pos_sta);
            res.push_back(temp);
            pos_sta = pos + 1;
        }
        res.push_back(fbo_info.substr(pos_sta));
        for(size_t i = 0; i < res.size(); i++){
            FBO_NATIVE_DBG_MSG("[FBO-NATIVE] fbo info is %s." , const_cast<char*>(res[i].c_str()));
        }
    } else {
        return -1;
    }

    return 0;
}

bool WriteFullyFd(int fd, const void* data, size_t byte_count) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    size_t remaining = byte_count;
    while (remaining > 0) {
        ssize_t n = TEMP_FAILURE_RETRY(write(fd, p, remaining));
        if (n == -1)
            return false;
        p += n;
        remaining -= n;
    }

    return true;
}

int Fbo_drop_caches (int caches_try){
    int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);

    if(fd >= 0){
        while(caches_try){
            write(fd, "3\n", 2);
            caches_try--;
            sync();
            sleep(1);
        } // drop caches_try times
        close(fd);

        return 0;
    } else{
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] DROP_CACHES fail!");
        return -1;
    }
}

string Fbo_Native_Socket(string file_zones){
    string res = "fail";
    const char * buf = file_zones.c_str();

    //receive parameter;
    char read_buf[128];
    memset(&read_buf, 0, sizeof(read_buf));
    char *p = &read_buf[0];
    int sum = 0, len = 0;

    FBO_NATIVE_INFO_MSG("[FBO-NATIVE] socket_local_client START\n");
    int mfd = socket_local_client(/*"/data/system/ndebugsocket"*/ "fbs_native_socket\0",
          /*ANDROID_SOCKET_NAMESPACE_FILESYSTEM*/ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (mfd == -1) {
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] unable to connect to mqsas native socket\n");
        return res;
    }
    struct timeval tv = {
        .tv_sec = 20 * 60,
        .tv_usec = 0,
    };
    FBO_NATIVE_INFO_MSG("[FBO-NATIVE] set send timeout START\n");
    if (setsockopt(mfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] failed to set send timeout on mqsas native socket\n");
        goto out;
    }

    FBO_NATIVE_INFO_MSG("[FBO-NATIVE] set send receive START\n");
    tv.tv_sec = 20 * 60 ;  // 5 mins on handshake read
    if (setsockopt(mfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] failed to set receive timeout on mqsas native socket\n");
        goto out;
    }
    FBO_NATIVE_INFO_MSG("[FBO-NATIVE] socket client success \n");

    //send buff
    if (!WriteFullyFd(mfd, buf, strlen(buf))) {
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] socket client write lba failed\n");
        goto out;
    }

    //receive buf
    while ((len = read(mfd, p + sum, sizeof(read_buf) - sum - 1)) > 0){
        sum += len;
        if (*(p+sum-1) == '}')
        break;
    }
    if ((sum > 0) && (*(p+sum-1) != '}')){
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] socket client read buff failed\n");
        goto out;
    } else {
        *(p+sum) = '\0';
        res = (string)read_buf;
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] socket client read buff success: %s\n", read_buf);
    }

out:
    close(mfd);
    return res;
}

bool FBO_F2FS_isSupport() {
    return true;
}

void FBO_F2FS_Trigger_Init(){
    fbo_state_flag = 0;
    current_file = "";
}

void FBO_F2FS_Trigger(const std::string& path) {

    int ret = 0, caches_try = 3; size_t pos = 0, pos_sta = 0, app_start_flag = 0, pre_pkg_start_flag = 0;
    int find_fbo_info_flag = 0;
    u32 file_nid = 0;
    u64 defrag_start = 0, defrag_length = 0;
    size_t each_start_ind = 0;
    time_t cur_time;
    int tm_day = 0;
    struct list_head zones_list;
    struct file_zone_entry *entry = NULL;
    vector<string> app_list;
    vector<string> fbo_info;
    vector<string> pre_pkg = {"/data/misc/profiles/cur/0/",
                              "/data/misc/profiles/ref/",
                              "/data/data/",
                              "/data/user_de/0/",
                              "/data/media/0/Android/data/",
                              "/data/media/0/Tencent/tbs/backup/",
                            };

    vector<string> other_path = {"/data/system/",
                            };

    get_time(cur_time, tm_day);
    if(pre_stop_time == 0 || cur_time - pre_stop_time > MIN_STOP_TIME)
        FBO_F2FS_Trigger_Init(); //trigger start recovery
    else
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] F2FS start abnomal");

    //Get all apps
    while((pos = path.find(",", pos+1)) != string::npos){
        string temp = path.substr(pos_sta, pos - pos_sta);
        app_list.push_back(temp);
        pos_sta = pos + 1;
    }
    app_list.push_back(path.substr(pos_sta));

    string dev_userdata =  get_userdata_mapper();

    ret = Get_FboInfo(fbo_info);
    if(ret){
        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] fbo info is empty.");
        ret = 0;
    } else {
        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] stop app is %s.", const_cast<char *>(fbo_info[APP_PKG].c_str()));
        for(size_t i = 0; i < app_list.size(); i++){
            if(fbo_info[APP_PKG].find(app_list[i]) != string::npos){ // get stop app
                FBO_NATIVE_DBG_MSG("[FBO-NATIVE] find app success");
                app_start_flag = i;
                find_fbo_info_flag = 1;
                break;
            }
        }
    }

    //INIT
    FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS init, %s !", const_cast<char *>(dev_userdata.c_str()));
    ret = f2fs_init_get_file_zones(const_cast<char *>(dev_userdata.c_str()));
    if(ret){
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] F2FS init or mount device failed");
        return;
    }

    //drop caches
    ret = Fbo_drop_caches (caches_try);
    if(ret){ //try
        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] Fbo_drop_caches failed");
        ret = Fbo_drop_caches (caches_try);
    }
    //Issue for each app
    for(size_t each = app_start_flag; each < app_list.size(); each++){
        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] pkg %s", const_cast<char *>(app_list[each].c_str()));
        vector<file_info> res;
        string file_zones = "";
        int start_file_index = 0;
        Before_Defrag_LBA_NUM = 0;
        After_Defrag_LBA_NUM = 0;
        bool try_LBA_socket = true, try_notify_socket = true;

        current_pkg = app_list[each];
        for (std::size_t i = 0; i < pre_pkg.size(); i++){
            FBO_NATIVE_INFO_MSG("[FBO-NATIVE] path %s", const_cast<char *>((pre_pkg[i] + app_list[each]).c_str()));
            get_all_file((pre_pkg[i] + app_list[each]).c_str(),res);
        } //Get all files

        // Other paths on the 1st of each month
        if(tm_day == 1 && (each == app_list.size() -1)){
            FBO_NATIVE_DBG_MSG("[FBO-NATIVE] start other path");
            for(std::size_t i = 0; i < other_path.size(); i++)
                get_all_file(other_path[i].c_str(),res);
        }

        // get stop index
        FBO_NATIVE_INFO_MSG("[FBO-NATIVE] get stop index");
        if(find_fbo_info_flag && each == app_start_flag){
            bool exist = true;
            int cut_count = 0;
            string fbo_stop_file = fbo_info[FBO_STOP_FILE];
            FBO_NATIVE_DBG_MSG("[FBO-NATIVE] fbo stop file is %s.", const_cast<char *>(fbo_stop_file.c_str()));
            string app_file;
            if(tm_day - atoi((fbo_info[FBO_STOP_DATE].substr(0,fbo_info[FBO_STOP_DATE].length()-1)).c_str()) >= APP_DEFRAG_TIME_LIMIT){
                FBO_NATIVE_DBG_MSG("[FBO-NATIVE] app %s already 3 days, return info to framework.", const_cast<char *>(fbo_info[FBO_STOP_DATE].c_str()));
                Before_Defrag_LBA_NUM = atoi(fbo_info[FRAGMENT_NUM_BEFOR_DEFRAG].c_str());
                After_Defrag_LBA_NUM = atoi(fbo_info[FRAGMENT_NUM_AFTER_DEFRAG].c_str());
                goto NOTIFY_STATUS;
            }
            while(exist){
                int temp = cut_count;
                while(temp){
                    fbo_stop_file = fbo_stop_file.substr(0, fbo_stop_file.find_last_of("/"));
                    temp--;
                }

                if(fbo_stop_file.find(app_list[each]) == string::npos)
                    break;

                for(std::size_t i = 0; i < res.size(); i++){
                    temp = cut_count;
                    app_file = res[i].file_path;
                    while(temp){
                        app_file = app_file.substr(0, app_file.find_last_of("/"));
                        temp--;
                    }
                    FBO_NATIVE_DBG_MSG("[FBO-NATIVE] stop file is %s.", const_cast<char *>(fbo_stop_file.c_str()));
                    FBO_NATIVE_DBG_MSG("[FBO-NATIVE] current file is %s.", const_cast<char *>(app_file.c_str()));

                    if(fbo_stop_file.compare(app_file) == 0){
                        exist = false;
                        start_file_index = i;
                        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] stop file index is %d.", start_file_index);
                        Before_Defrag_LBA_NUM = atoi(fbo_info[FRAGMENT_NUM_BEFOR_DEFRAG].c_str());
                        After_Defrag_LBA_NUM = atoi(fbo_info[FRAGMENT_NUM_AFTER_DEFRAG].c_str());
                        break;
                    }
                }
                cut_count++;
            }
        }
        /*
         * For app all files
         * Processing flow
            * 50 files at a time
            * F2FS get filezones
            * F2FS defrag
                * DROP cache
                * F2FS get filezones
        */
        for (std::size_t i = start_file_index; i < res.size(); i++) {
            current_file = res[i].file_path;
            if(fbo_state_flag){
                FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS the stop file index is %d.", i);
                FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS stop get lba");
                goto out;
            }
            FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS the file number is %d", res.size());
            FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS APP_PATH %s", const_cast<char *>(res[i].file_path.c_str()));
            //DUMP LBA
            file_nid = atoi(res[i].file_inode.c_str());
            FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS DUMP LBA! %d", file_nid);

            zones_list = LIST_HEAD_INIT(zones_list);
            ret = dump_file_range(file_nid, &zones_list, 1);
            if(ret == NO_NEED){
                FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS no need defragment!");
                continue;
            }else if(ret == FAILED){
                FBO_NATIVE_ERR_MSG("[FBO-NATIVE] F2FS failed to get ranger");
                continue;
            }

            list_for_each_entry(entry, &zones_list, list){
                if(entry->start_blkaddr != NULL_ADDR
            	&& entry->start_blkaddr != NEW_ADDR
            	&& entry->range != 0){
                    Before_Defrag_LBA_NUM ++;
                    file_zones = file_zones + "0x" + ito_16str(entry->start_blkaddr) + ",0x" + ito_16str(entry->start_blkaddr + entry->range - (u64)1) + ",";
                }
            } //Before F2FS defrag lba zone
            FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS DUMP LBA before is: %s", const_cast<char *>(file_zones.c_str()));
            file_zones.clear();
            destory_file_zones(&zones_list);

            //Defrag file
            if(atoll(res[i].file_size.c_str()) % ALIGNMENT_RULE ){
                defrag_length = atoll(res[i].file_size.c_str()) + (ALIGNMENT_RULE - (atoll(res[i].file_size.c_str()) % ALIGNMENT_RULE));
            }else
                defrag_length = atoll(res[i].file_size.c_str());

            ret = defrag_file(defrag_start, defrag_length, const_cast<char *>(res[i].file_path.c_str()));
            if(ret){
                FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS F2FS_IOC_DEFRAGMENT!");
            } else{
                FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS DEFRAGE: %s success!", res[i].file_path.c_str());
            }

            //Send 20 file data to HAL
            if( ((i+1) % EACH_FILE_NUM == 0) || i == res.size()-1 ){
                //drop cache
                FBO_NATIVE_INFO_MSG("[FBO-NATIVE] DROP_CACHES!");
                ret = Fbo_drop_caches (caches_try);
                if(ret) // try
                    ret = Fbo_drop_caches (caches_try);
                //Record LBA after defrag
                try_LBA_socket = true;

                if((i+1) % EACH_FILE_NUM == 0)
                    each_start_ind = ((i+1) / EACH_FILE_NUM - 1) * EACH_FILE_NUM;
                else
                    each_start_ind = ((i+1) / EACH_FILE_NUM) * EACH_FILE_NUM;
LBA_TO_HAL:
                for( ; each_start_ind <= i; each_start_ind++){
                    string temp = "";

                    if(fbo_state_flag){
                        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS stop defrag");
                        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS the stop file index is %d.", i);
                        goto out;
                    }

                    file_nid = atoi(res[each_start_ind].file_inode.c_str());
                    zones_list = LIST_HEAD_INIT(zones_list);
                    ret = dump_file_range(file_nid, &zones_list, 1);
                    if(ret == NO_NEED){
                        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS no need defragment!");
                        continue;
                    } else if(ret == FAILED){
                        FBO_NATIVE_ERR_MSG("[FBO-NATIVE] F2FS failed to get ranger");
                        continue;
                    }

                    list_for_each_entry(entry, &zones_list, list){
                        if(entry->start_blkaddr != NULL_ADDR
            	        && entry->start_blkaddr != NEW_ADDR
                        && entry->range != 0){
                            After_Defrag_LBA_NUM ++;
                            temp = temp + "0x" + ito_16str(entry->start_blkaddr) + ",0x" + ito_16str(entry->start_blkaddr + entry->range - (u64)1) + ",";
                            if(entry->range > MIN_LBA_SIZE)
                                file_zones = file_zones + "0x" + ito_16str(entry->start_blkaddr) + ",0x" + ito_16str(entry->start_blkaddr + entry->range - (u64)1) + ",";
                        }
                    }

                    destory_file_zones(&zones_list);
                    FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS DUMP LBA after is: %s", const_cast<char *>(temp.c_str()));
                }

                file_zones = file_zones + "}"; //socket end flag
                FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS to HAL LBA: %s", const_cast<char *>(file_zones.c_str()));
                //socket
                string socket_flag = Fbo_Native_Socket(file_zones);

                if(socket_flag.find("fail") != string::npos){
                    FBO_NATIVE_ERR_MSG("[FBO-NATIVE] socket fail");
                    if(try_LBA_socket){
                        try_LBA_socket = false;
                        goto LBA_TO_HAL;
                    }
                }
                FBO_NATIVE_DBG_MSG("[FBO-NATIVE] socket success");
                file_zones.clear();
            }
        }

        try_notify_socket = true;

NOTIFY_STATUS:
        //notify current app status   pkg:PKG:F2FS:123:456
        FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS notify %s status.", const_cast<char *>(app_list[each].c_str()));

        /* temporary workround
         * Avoid negative numbers caused by failed drop cache of new files
        */
        if(Before_Defrag_LBA_NUM < After_Defrag_LBA_NUM){
            FBO_NATIVE_ERR_MSG("[FBO-NATIVE] the number of fragment becomes larger after defrag ");
            After_Defrag_LBA_NUM = Before_Defrag_LBA_NUM;
        }

        string notify_info = "pkg:" + app_list[each] + ":F2FS:" + to_string(Before_Defrag_LBA_NUM) + ":" + to_string(After_Defrag_LBA_NUM) + "}";

        FBO_NATIVE_INFO_MSG("[FBO-NATIVE] F2FS  %s .", const_cast<char *>(notify_info.c_str()));
        string socket_info = Fbo_Native_Socket(notify_info);

        if(socket_info.find("fail") != string::npos){
            FBO_NATIVE_ERR_MSG("[FBO-NATIVE] notify socket fail");
            if(try_LBA_socket){
                try_LBA_socket = false;
                goto NOTIFY_STATUS;
            }
        }

        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS APP %s processing complete .", const_cast<char *>(app_list[each].c_str()));

    }

    FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS all APP processing complete .");

out:
    f2fs_exit_get_file_zones();

}

string FBO_Native_State(const std::string& cmd){
    string fbo_state = cmd;
    string res = "success";
    int ret = 0;
    int tm_day;
    vector<string> file_info_;

    if(fbo_state.find("stop") != string::npos){ //stop
        fbo_state_flag = 1;
        res = current_file;
        get_time(pre_stop_time, tm_day);
        FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS stop, info: %s", const_cast<char *>(res.c_str()));
        ret = Get_FboInfo(file_info_);
        if(ret || file_info_[APP_PKG].find(current_pkg) == string::npos){
            FBO_NATIVE_DBG_MSG("[FBO-NATIVE] update tm_day");
            Set_FboInfo(tm_day);
        }else {
            FBO_NATIVE_DBG_MSG("[FBO-NATIVE] do not update tm_day");
            Set_FboInfo(atoi((file_info_[FBO_STOP_DATE].substr(0, file_info_[FBO_STOP_DATE].length()-1)).c_str()));
        }
    } else if(fbo_state.find("continue") != string::npos){ // continue,file
        fbo_state_flag = 0;
        if(fbo_state.find(",") != string::npos){
            start_file = fbo_state.substr(fbo_state.find(",") + 1);
            FBO_NATIVE_DBG_MSG("[FBO-NATIVE] F2FS continue,info: %s", const_cast<char *>(fbo_state.c_str()));
        }
    }

    return res;
}