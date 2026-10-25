/*
 * Copyright (c) 2015, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXTENDED_ARTSP_CONNECTION_H_
#define EXTENDED_ARTSP_CONNECTION_H_

namespace android {

struct ExtendedARTSPConnection : public ARTSPConnection {
    ExtendedARTSPConnection(bool uidValid, uid_t uid);

    virtual bool isIPV6();
protected:
    virtual ~ExtendedARTSPConnection();
    virtual void performConnect(const sp<AMessage> &reply,
            AString host, unsigned port);
    virtual void performCompleteConnection(const sp<AMessage> &msg,
            int err);
    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
    enum {
        kWhatReconnect = 'reco',
    };
    void *mAddrHeader;
    bool mIsIPV6;

    bool createSocketAndConnect(void *res, unsigned port, const sp<AMessage> &reply);
    void onReconnect(const sp<AMessage> &msg);
};

} // namespace android
#endif // EXTENDED_ARTSP_CONNECTION_H_
