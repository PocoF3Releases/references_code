/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
       * Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
         copyright notice, this list of conditions and the following
         disclaimer in the documentation and/or other materials provided
         with the distribution.
       * Neither the name of The Linux Foundation nor the names of its
         contributors may be used to endorse or promote products derived
         from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/

#define LOG_TAG "BluetoothAPM_Jni"

#define LOG_NDEBUG 0

#include "android_runtime/AndroidRuntime.h"
#include "com_android_bluetooth.h"
#include "hardware/bt_apm.h"
#include "utils/Log.h"

#include <string.h>
#include <shared_mutex>

namespace android {
static jmethodID method_onGetActiveprofileCallback;

static const bt_apm_interface_t* sBluetoothApmInterface = nullptr;
static std::shared_timed_mutex interface_mutex;

static jobject mCallbacksObj = nullptr;
static std::shared_timed_mutex callbacks_mutex;


static int btapm_active_profile_callback(const RawAddress& bd_addr, uint16_t audio_type)
{
  ALOGI("%s", __func__);
  std::shared_lock<std::shared_timed_mutex> lock(callbacks_mutex);
  CallbackEnv sCallbackEnv(__func__);
  if (!sCallbackEnv.valid() || mCallbacksObj == nullptr) return -1;

  ScopedLocalRef<jbyteArray> addr(
        sCallbackEnv.get(), sCallbackEnv->NewByteArray(sizeof(RawAddress)));
  if (!addr.get()) {
  ALOGE("%s: Fail to new jbyteArray bd addr", __func__);
  return -1;
  }

  sCallbackEnv->SetByteArrayRegion(
          addr.get(), 0, sizeof(RawAddress),
          reinterpret_cast<const jbyte*>(bd_addr.address));
  return sCallbackEnv->CallIntMethod(mCallbacksObj, method_onGetActiveprofileCallback,
                                                            addr.get(), (jint)audio_type);
}


static btapm_initiator_callbacks_t sBluetoothApmCallbacks = {
        sizeof(sBluetoothApmCallbacks),
        btapm_active_profile_callback
};

static void classInitNative(JNIEnv* env, jclass clazz) {

  ALOGI("%s: succeeds", __func__);
  method_onGetActiveprofileCallback =
     env->GetMethodID(clazz, "getActiveProfile", "([BI)I");
}

static bool initNative(JNIEnv* env, jobject object) {
  std::unique_lock<std::shared_timed_mutex> interface_lock(interface_mutex);
  std::unique_lock<std::shared_timed_mutex> callbacks_lock(callbacks_mutex);

  const bt_interface_t* btInf = getBluetoothInterface();
  if (btInf == nullptr) {
    ALOGE("%s: Bluetooth module is not loaded", __func__);
    return JNI_FALSE;
  }

  if (sBluetoothApmInterface != nullptr) {
    ALOGW("%s: Cleaning up APM Interface before initializing...", __func__);
    sBluetoothApmInterface->cleanup();
    sBluetoothApmInterface = nullptr;
  }

  if (mCallbacksObj != nullptr) {
    ALOGW("%s: Cleaning up APM callback object", __func__);
    env->DeleteGlobalRef(mCallbacksObj);
    mCallbacksObj = nullptr;
  }

  if ((mCallbacksObj = env->NewGlobalRef(object)) == nullptr) {
    ALOGE("%s: Failed to allocate Global Ref for APM Callbacks", __func__);
    return JNI_FALSE;
  }

  sBluetoothApmInterface =
      (bt_apm_interface_t*)btInf->get_profile_interface(
          BT_APM_MODULE_ID);
  if (sBluetoothApmInterface == nullptr) {
    ALOGE("%s: Failed to get Bluetooth APM Interface", __func__);
    return JNI_FALSE;
  }
  bt_status_t status = sBluetoothApmInterface->init(&sBluetoothApmCallbacks);
  if (status != BT_STATUS_SUCCESS) {
    ALOGE("%s: Failed to initialize Bluetooth APM, status: %d", __func__,
          status);
    sBluetoothApmInterface = nullptr;
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

static void cleanupNative(JNIEnv* env, jobject object) {
  std::unique_lock<std::shared_timed_mutex> interface_lock(interface_mutex);
  std::unique_lock<std::shared_timed_mutex> callbacks_lock(callbacks_mutex);

  const bt_interface_t* btInf = getBluetoothInterface();
  if (btInf == nullptr) {
    ALOGE("%s: Bluetooth module is not loaded", __func__);
    return;
  }

  if (sBluetoothApmInterface != nullptr) {
    sBluetoothApmInterface->cleanup();
    sBluetoothApmInterface = nullptr;
  }

  if (mCallbacksObj != nullptr) {
    env->DeleteGlobalRef(mCallbacksObj);
    mCallbacksObj = nullptr;
  }
}

static jboolean activeDeviceUpdateNative(JNIEnv* env, jobject object,
                            jbyteArray address, jint profile, jint audio_type) {
  ALOGI("%s: sBluetoothApmInterface: %p", __func__, sBluetoothApmInterface);
  std::shared_lock<std::shared_timed_mutex> lock(interface_mutex);
  if (!sBluetoothApmInterface) {
    ALOGE("%s: Failed to get the Bluetooth APM Interface", __func__);
    return JNI_FALSE;
  }

  jbyte* addr = env->GetByteArrayElements(address, nullptr);

  RawAddress bd_addr = RawAddress::kEmpty;
  if (addr) {
    bd_addr.FromOctets(reinterpret_cast<const uint8_t*>(addr));
  }
  bt_status_t status = sBluetoothApmInterface->active_device_change(bd_addr, profile, audio_type);
  if (status != BT_STATUS_SUCCESS) {
    ALOGE("%s: Failed APM active_device_change, status: %d", __func__, status);
  }
  env->ReleaseByteArrayElements(address, addr, 0);
  return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}

static jboolean setContentControlNative(JNIEnv* env, jobject object,
                                      jint content_control_id, jint profile) {
  ALOGI("%s: sBluetoothApmInterface: %p", __func__, sBluetoothApmInterface);
  std::shared_lock<std::shared_timed_mutex> lock(interface_mutex);
  if (!sBluetoothApmInterface) {
    ALOGE("%s: Failed to get the Bluetooth APM Interface", __func__);
    return JNI_FALSE;
  }

  bt_status_t status = sBluetoothApmInterface->set_content_control_id(content_control_id, profile);
  if (status != BT_STATUS_SUCCESS) {
    ALOGE("%s: Failed APM content control update, status: %d", __func__, status);
  }

  return (status == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;
}


static jboolean isQcLeaEnabledNative(JNIEnv* env, jobject object) {
  ALOGI("%s: sBluetoothApmInterface: %p", __func__, sBluetoothApmInterface);
  std::shared_lock<std::shared_timed_mutex> lock(interface_mutex);
  if (!sBluetoothApmInterface) {
    ALOGE("%s: Failed to get the QC_LEA_Enabled Value", __func__);
    return JNI_FALSE;
  }
  jboolean status = (jboolean)(sBluetoothApmInterface->is_qclea_enabled());
  return status;
}

static jboolean isAospLeaEnabledNative(JNIEnv* env, jobject object) {
  ALOGI("%s: sBluetoothApmInterface: %p", __func__, sBluetoothApmInterface);
  std::shared_lock<std::shared_timed_mutex> lock(interface_mutex);
  if (!sBluetoothApmInterface) {
    ALOGE("%s: Failed to get the QC_LEA_Enabled Value", __func__);
    return JNI_FALSE;
  }
  jboolean status = (jboolean)(sBluetoothApmInterface->is_aospLea_enabled());
  return status;
}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void*)classInitNative},
    {"initNative", "()V", (void*)initNative},
    {"cleanupNative", "()V", (void*)cleanupNative},
    {"activeDeviceUpdateNative", "([BII)Z", (void*)activeDeviceUpdateNative},
    {"setContentControlNative", "(II)Z", (void*)setContentControlNative},
    {"isQcLeaEnabledNative", "()Z", (void*)isQcLeaEnabledNative},
    {"isAospLeaEnabledNative", "()Z", (void*)isAospLeaEnabledNative},
};

int register_com_android_bluetooth_apm(JNIEnv* env) {
  return jniRegisterNativeMethods(
      env, "com/android/bluetooth/apm/ApmNativeInterface", sMethods,
      NELEM(sMethods));
}
}
