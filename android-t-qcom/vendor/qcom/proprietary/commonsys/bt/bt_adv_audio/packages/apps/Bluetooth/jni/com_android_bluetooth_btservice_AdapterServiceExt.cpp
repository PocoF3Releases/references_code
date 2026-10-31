/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/******************************************************************************
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *      * Neither the name of The Linux Foundation nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include "android_runtime/AndroidRuntime.h"
#include "com_android_bluetooth_ext.h"

namespace android {

  int register_com_android_bluetooth_adv_audio_profiles(JNIEnv* env) {
    ALOGE("%s", __func__);

    int status = android::register_com_android_bluetooth_acm(env);
    if (status < 0) {
      ALOGE("jni acm registration failure: %d", status);
      return JNI_ERR;
    }

    status = android::register_com_android_bluetooth_apm(env);
    if (status < 0) {
      ALOGE("jni APM registration failure: %d", status);
      return JNI_ERR;
    }

    status = android::register_com_android_bluetooth_bap_broadcast(env);
    if (status < 0) {
      ALOGE("jni bap broadcast registration failure: %d", status);
      return JNI_ERR;
    }

    status = android::register_com_android_bluetooth_vcp_controller(env);
    if (status < 0) {
      ALOGE("jni vcp controller registration failure: %d", status);
      return JNI_ERR;
    }

    status = android::register_com_android_bluetooth_pacs_client(env);
    if (status < 0) {
      ALOGE("jni pacs client registration failure: %d", status);
      return JNI_ERR;
    }
    status = android::register_com_android_bluetooth_call_controller(env);
    if (status < 0) {
      ALOGE("jni CC registration failure: %d", status);
      return JNI_ERR;
    }

    status = android::register_com_android_bluetooth_mcp(env);
    if (status < 0) {
      ALOGE("jni mcp registration failure: %d", status);
      return JNI_ERR;
    }
    return JNI_VERSION_1_6;
  }
}
