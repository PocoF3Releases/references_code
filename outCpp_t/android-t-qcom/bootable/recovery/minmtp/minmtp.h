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
#include <pthread.h>
#include <android-base/logging.h>

#include "MtpTypes.h"
#include "MtpServer.h"
#include "MtpStorage.h"
#include "MinMtpDatabase.h"

#ifndef MINMTP_H
#define MINMTP_H

using namespace android;
class MinMtp{

public:
    /**
     * setup MTP for data tansformation.
     *
     * @param   path            the path to mount as storage root.
     *
     * @retval  true   succeed
     *          false  failed
     */
    bool enable_mtp(const char * path);

    /**
     * shutdown mtp.
     *
     * @retval  true   succeed
     *          false  failed
     */
    bool disable_mtp(void);

    /**
     * get mtp status.
     *
     * @retval  true   enabled
     *          false  disabled
     */
    bool is_enable()
    {
        return is_enabled;
    }

    /**
      * get the first file of mtp storage.
      * used for OTA installing.
      *
      * @retval        the path if succeed
      *          NULL  failed
      */
    const char * get_file_path();

    MinMtp();

    ~MinMtp();

private:
    /** storage list for mtp, at most one storage.*/
    MtpStorageList       mstorage;

    /** mtp server*/
    MtpServer*           mserver;

    /** mtp database, i.e. the file structure for mtp.*/
    MinMtpDatabase*       mtpdb;

    /** run mtp server in new thread.*/
    pthread_t            mtpthread;

    /** file descriptor for mtp ffs endpoint*/
    int                  m_fd_ffs_mtp_ep0;

    /** mtp enable state*/
    bool                 is_enabled;

private:
    /**
     * set up server and start.
     */
    static void * start_server(void * arg);

    /**
     * add storage to MTP server.
     *
     * @param  storage_name   name of storage
     * @param  storage_path   path
     * @param  storage_id     id
     */
    bool add_storage(const char* storage_name, const char* storage_path, int storage_id);

    /**
     * start mtp server in new thread.
     *
     * @retval mtp server thread.
     */
    pthread_t run_server(void);

};

#endif //MINMTP_H
