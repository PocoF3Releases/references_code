/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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


#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <cutils/properties.h>
#include <android-base/logging.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "MtpDataPacket.h"
#include "MinMtpDatabase.h"
#include "minmtp.h"

using namespace android;
#define FFS_MTP_EP0 "/dev/usb-ffs/mtp/ep0"

MinMtp::MinMtp(){
    is_enabled = false;
    mtpdb = NULL;
    mtpthread = 0;
    mserver = NULL;
    m_fd_ffs_mtp_ep0 = 0;
}

MinMtp::~MinMtp(){
        disable_mtp();
}

const char * MinMtp::get_file_path(){
    if(is_enabled) {
        if(mtpdb) {
            return mtpdb->get_file_path();
        }
    }
    return NULL;
}

pthread_t MinMtp::run_server(){
    //run server in new thread.
    pthread_t thread;
    pthread_create(&thread, NULL, (MinMtp::start_server), this);
    return thread;
}

void * MinMtp::start_server(void * arg){
    LOG(INFO) << "starting MTP server.";

    //set proc name for compatibility with MTK device.
    prctl(PR_SET_NAME, "MtpServer");

    MinMtp * pthis = (MinMtp *) arg;
    //1.set up mtp database
    pthis->mtpdb = new MinMtpDatabase(&pthis->mstorage);

    //2.set up mtp server
    bool usePtp =  false;
    pthis->mserver = new MtpServer(pthis->mtpdb, pthis->m_fd_ffs_mtp_ep0, usePtp, "", "", "", "");
    if (pthis->mserver)
        LOG(ERROR) << "created new mtpserver object";
    else {
        LOG(ERROR) << "creat new mtpserver object failed";
        return NULL;
    }

    //3.add storage to mtp server
    for (unsigned int i = 0; i < pthis->mstorage.size(); ++i) {
        pthis->mserver->addStorage( (pthis->mstorage)[i]);
    }

    //4.run server
    if((pthis->mstorage)[0] && pthis->mserver)
        pthis->mserver->run();

    return NULL;
}


bool MinMtp::add_storage(const char* storage_name, const char* storage_path, int storage_id){
    LOG(INFO) << "adding path: " << storage_path << " as mtp storage:" << storage_name << ", with id: " << storage_id;
    if (storage_path) {
        int storageID = storage_id;
        bool removable = false;
        long long maxFileSize = 10000000000L;
        if (storage_name) {
            MtpStorage * storage = new MtpStorage(storageID, storage_path, storage_name, removable, maxFileSize);
            mstorage.push_back(storage);
        } else{
            LOG(ERROR) << "Invalid storage name when add_storage for minmtp";
            return false;
        }
    } else {
        LOG(ERROR) << "Invalid storage path when add_storage for minmtp";
        return false;
    }
    return true;
}

bool MinMtp::enable_mtp(const char * path){

    if (!is_enabled){
        LOG(INFO) << "Starting MTP";

        // open mtp ffs endpoint
        m_fd_ffs_mtp_ep0 = TEMP_FAILURE_RETRY(open(FFS_MTP_EP0, O_RDWR));
        if (m_fd_ffs_mtp_ep0 < 0) {
            LOG(ERROR) << "could not open MTP control, errno: " << strerror(errno);
        }

        //add "/data" partition as mtp storage.
        if(false == add_storage("data", path, 1)){
            LOG(ERROR) << "Add storage for mtp failed!";
            return false;
        }

        //start mtp server
        mtpthread = run_server();
        if(0 == mtpthread ){
            LOG(ERROR) << "Start mtp server failed!";
            return false;
        }
    } else {
        LOG(INFO) << "MTP is already enabled!";
    }
    is_enabled = true;
    return true;
}

bool MinMtp::disable_mtp(){

    //1.remove storage in server.
    while(!mstorage.empty()){
        std::vector<MtpStorage *>::iterator last = mstorage.begin();
        if(*last){
            delete *last;
            mstorage.erase(last);
        }
    }

    //2.kill mtp server
    if (mtpthread != 0) {
        if(!pthread_kill(mtpthread, 0)){
            void * res = NULL;
            int s = pthread_join(mtpthread, &res);
            if (s != 0){
                printf("jion error: %d\n", errno);
            }
            if (res) {
                printf("Joined with thread %ld; returned value was %s\n",
                mtpthread, (char *) res);
                free(res);  
            }
       } else{
            printf("Maybe Mtpthread is dead. error: %d\n", errno);
       }
       mtpthread = 0;
   }

    //3.delete objects for mtp
    if (mtpdb) {
        delete mtpdb;
        mtpdb = NULL;
    }
    if(mserver) {
        delete mserver;
        mserver = NULL;
    }

    LOG(ERROR) << "MTP is disabled!";
    is_enabled = false;
    return true;
}
