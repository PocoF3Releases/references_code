/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
Copyright (c) 2020-2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

              Diag communication support

GENERAL DESCRIPTION

Implementation of diag hidl communication layer between diag library and diag driver.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#include <stdlib.h>
#include "comdef.h"
#include "stdio.h"
#include "stringl.h"
#include "diag_lsmi.h"
#include "diagsvc_malloc.h"
#include "diag_lsm_event_i.h"
#include "diag_lsm_log_i.h"
#include "diag_lsm_msg_i.h"
#include "diag.h" /* For definition of diag_cmd_rsp */
#include "diag_lsm_pkt_i.h"
#include "diag_lsm_dci_i.h"
#include "diag_lsm_dci.h"
#include "diag_shared_i.h" /* For different constants */
#include <diag_lsm.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "errno.h"
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <vendor/qti/diaghal/1.0/Idiag.h>
#include <vendor/qti/diaghal/1.0/Idiagcallback.h>
#include <vendor/qti/diaghal/1.0/types.h>
#include <hidl/Status.h>
#include <utils/misc.h>
#include <utils/Log.h>
#include <hidl/HidlSupport.h>
#include <stdio.h>
#include <log/log.h>
#include <hidl/Status.h>
#include <hwbinder/IPCThreadState.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <hidl/LegacySupport.h>
#include <pthread.h>
#include "diagcallback.h"
#include "diag_lsm_hidl_client.h"
unsigned int data_len = 0;

using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using android::sp;
using ::android::hardware::hidl_handle;
using vendor::qti::diaghal::V1_0::Idiag;
using vendor::qti::diaghal::V1_0::Idiagcallback;
using vendor::qti::diaghal::V1_0::implementation::diagcallback;
using ::android::hardware::hidl_memory;
using ::android::hardware::HidlMemory;
sp<IAllocator> ashmemAllocator;
using android::hardware::Void;
using android::hardware::Return;
using android::hardware::hidl_string;
using android::hardware::hidl_death_recipient;
using android::hidl::base::V1_0::IBase;

int diag_use_dev_node = 0;
sp<Idiag> mDiagClient;
static pthread_mutex_t m_diag_client_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t read_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t read_data_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t write_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t write_data_cond = PTHREAD_COND_INITIALIZER;
int read_buf_busy = 0;

size_t memscpy(void *dst, size_t dst_size, const void *src, size_t copy_size);

struct diaghalDeathRecipient : virtual public hidl_death_recipient {
     //called when the interface linked to it dies
     virtual void serviceDied(uint64_t, const android::wp<IBase>&) override {
         DIAG_LOGE("diaghal just died");
	 pthread_mutex_lock(&m_diag_client_mutex);
         mDiagClient  = nullptr;
	 pthread_mutex_unlock(&m_diag_client_mutex);

	diag_kill_comm_read();
     }
};

size_t memscpy(void *dst, size_t dst_size, const void *src, size_t copy_size)
{
    if(dst_size >= copy_size) {
#ifdef _WIN32
        memcpy_s(dst, dst_size, src, copy_size);
#else
        memcpy(dst, src, copy_size);
#endif
        return copy_size;
    }

#ifdef _WIN32
    memcpy_s(dst, dst_size, src, dst_size);
#else
    memcpy(dst, src, dst_size);
#endif

    return dst_size;
}

static android::sp<diaghalDeathRecipient> halDeathRecipient = nullptr;
sp<Idiagcallback> callback;
/*===========================================================================

FUNCTION diag_lsm_comm_open

DESCRIPTION
  If /dev/diag exists opens fd to /dev/diag else it tries to get hidl server instance
  and registers callback with server

DEPENDENCIES
   None

RETURN VALUE
  SUCCESS/FAILURE.

SIDE EFFECTS
  None.
===========================================================================*/
int diag_lsm_comm_open(void)
{
	int fd = -1;
	int ret;

	fd = open("/dev/diag", O_RDWR | O_CLOEXEC);
	if (fd >= 0) {
		diag_use_dev_node = 1;
	} else {
		if (errno == ENOENT) {
			diag_use_dev_node = 0;
			mDiagClient = Idiag::getService();
			if (mDiagClient != NULL) {
		             if(halDeathRecipient == nullptr) {
				halDeathRecipient = new diaghalDeathRecipient();
			     }
				Return<bool> death_linked = mDiagClient->linkToDeath(halDeathRecipient, 0);
				if(!death_linked || !death_linked.isOk()) {
					DIAG_LOGE("linking diaghal to death failed: %s",death_linked.description().c_str());
					mDiagClient = nullptr;
					return fd;
				}
				DIAG_LOGE("diaghal linked to death!!");
				callback = diagcallback::getInstance();
                                mDiagClient->open(callback);
				ashmemAllocator = IAllocator::getService("ashmem");
			        DIAG_LOGE("diag:successfully connected to service \n");
                        }
                        else {
                                DIAG_LOGE("diag: Unable to connect to hidl server\n");
				return fd;
			}
			fd = 0;
		}
	}
	return fd;
}

static inline int __diag_lsm_comm_ioctl(unsigned long request, void *dst, size_t dst_size,
					void *src, size_t copy_size)
{
	int ret = 0;
	void *data = NULL;

	ashmemAllocator->allocate(copy_size, [&](bool success, const hidl_memory &mem) {
		if (!success) {
			DIAG_LOGE("ashmem allocate failed!!");
			ret = -1;
			return;
		}
		sp<IMemory> memory = mapMemory(mem);
		if (!memory) {
			DIAG_LOGE("%s: Could not map HIDL memory to IMemory", __func__);
			ret = -1;
			return;
		}
		data = memory->getPointer();
		if (!data) {
			DIAG_LOGE("%s: Could not get pointer to memory", __func__);
			ret = -1;
			return;
		}
		/* copy data to diag-router */
		if (src) {
			memory->update();
			memcpy(data, src, copy_size);
			memory->commit();
		}
		pthread_mutex_lock(&m_diag_client_mutex);
		if (!mDiagClient) {
			ret = -1;
			pthread_mutex_unlock(&m_diag_client_mutex);
			return;
		}
		auto status = mDiagClient->ioctl(request, mem, copy_size);
		pthread_mutex_unlock(&m_diag_client_mutex);
		if (!status.isOk()) {
			DIAG_LOGE("%s: Error for ioctl req: %d\n", __func__, request);
			ret = -1;
			return;
		}
		ret = status;
		/* copy data from diag-router */
		if (dst)
			memscpy(dst, dst_size, data, copy_size);
	});

	return ret;
}

/*===========================================================================

FUNCTION diag_lsm_comm_ioctl

DESCRIPTION
  If /dev/diag exists calls ioctl to kernel diag driver else tries to send
  ioctl command to diag hidl server

DEPENDENCIES
   None

RETURN VALUE
  SUCCESS/FAILURE.

SIDE EFFECTS
  None.
===========================================================================*/
int diag_lsm_comm_ioctl(int fd, unsigned long request, void *buf, unsigned int len)
{
	int i = 0, num_bytes = 0, err = 0;
	hidl_memory mem_s;

	if (diag_use_dev_node) {
		err = ioctl(fd, request, buf, len, NULL, 0, NULL, NULL);
		return err;
	}

	if (ashmemAllocator == NULL)
		return -ENOMEM;

	switch (request) {
	case DIAG_IOCTL_COMMAND_REG:
	{
		struct diag_cmd_tbl {
			int count;
			diag_cmd_reg_entry_t entries[0];
		} __packed;
		struct diag_cmd_tbl *tbl ;
		diag_cmd_reg_tbl_t *reg_tbl;

		reg_tbl = (diag_cmd_reg_tbl_t *)buf;
		num_bytes = (sizeof(diag_cmd_reg_entry_t) * reg_tbl->count) + sizeof(reg_tbl->count);
		tbl = (struct diag_cmd_tbl *)malloc(num_bytes);
		if (!tbl)
			return -ENOMEM;
		tbl->count = reg_tbl->count;
		for (i = 0; i < reg_tbl->count; i++) {
			tbl->entries[i].cmd_code = reg_tbl->entries[i].cmd_code;
			tbl->entries[i].subsys_id = reg_tbl->entries[i].subsys_id;
			tbl->entries[i].cmd_code_lo = reg_tbl->entries[i].cmd_code_lo;
			tbl->entries[i].cmd_code_hi = reg_tbl->entries[i].cmd_code_hi;
		}

		err = __diag_lsm_comm_ioctl(request, NULL, 0, tbl, num_bytes);
		break;
	}
	case DIAG_IOCTL_COMMAND_DEREG:
	case DIAG_IOCTL_QUERY_MASK:
	{
		pthread_mutex_lock(&m_diag_client_mutex);
		if (!mDiagClient) {
			pthread_mutex_unlock(&m_diag_client_mutex);
			return -1;
		}
		auto ret = mDiagClient->ioctl(request, mem_s, num_bytes);
		pthread_mutex_unlock(&m_diag_client_mutex);
		if (!ret.isOk()) {
			DIAG_LOGE("diag: Failed to send ioctl req: %d\n", request);
			return -1;
		}
		err = ret;
		break;
	}
	case DIAG_IOCTL_LSM_DEINIT:
	{
		pthread_mutex_lock(&m_diag_client_mutex);
		if (!mDiagClient) {
			pthread_mutex_unlock(&m_diag_client_mutex);
			return -1;
		}
		auto ret = mDiagClient->ioctl(request, mem_s, num_bytes);
		if (!ret.isOk()) {
			DIAG_LOGE("diag: Failed to send ioctl req: %d\n", request);
			pthread_mutex_unlock(&m_diag_client_mutex);
			return -1;
		}
		mDiagClient->unlinkToDeath(halDeathRecipient);
		pthread_mutex_unlock(&m_diag_client_mutex);
		if (ret >= 0)
			return 1;
		err = ret;
		break;
	}
	case DIAG_IOCTL_GET_DELAYED_RSP_ID:
		num_bytes = 2;
		err  = __diag_lsm_comm_ioctl(request, buf, len, NULL, num_bytes);
		break;
	case DIAG_IOCTL_REMOTE_DEV:
		num_bytes = 4;
		err = __diag_lsm_comm_ioctl(request, buf, len, NULL, num_bytes);
		break;
	case DIAG_IOCTL_QUERY_CON_ALL:
		num_bytes = (sizeof(struct diag_con_all_param_t));
		err = __diag_lsm_comm_ioctl(request, buf, len, NULL, num_bytes);
		break;
	case DIAG_IOCTL_QUERY_MD_PID:
	{
		struct diag_query_pid_t *pid_params = (struct diag_query_pid_t *)buf;

		num_bytes = (sizeof(struct diag_query_pid_t));
		err = __diag_lsm_comm_ioctl(request, buf, len, pid_params, num_bytes);
		break;
	}
	case DIAG_IOCTL_SWITCH_LOGGING:
	{
		int pid = getpid();
		int total_len = len + sizeof(pid);
		unsigned char tmp_buf[total_len];

		memcpy(tmp_buf, buf, len);
		memcpy(tmp_buf + len, &pid, sizeof(pid));
		err = __diag_lsm_comm_ioctl(request, NULL, 0, tmp_buf, total_len);
		break;
	}
	case DIAG_IOCTL_DCI_SUPPORT:
	case DIAG_IOCTL_DCI_HEALTH_STATS:
	case DIAG_IOCTL_DCI_LOG_STATUS:
	case DIAG_IOCTL_DCI_EVENT_STATUS:
	case DIAG_IOCTL_DCI_CLEAR_LOGS:
	case DIAG_IOCTL_DCI_CLEAR_EVENTS:
	case DIAG_IOCTL_DCI_DEINIT:
		err = __diag_lsm_comm_ioctl(request, buf, len, buf, len);
		return err;
	case DIAG_IOCTL_DCI_REG:
		err = __diag_lsm_comm_ioctl(request, NULL, 0, buf, len);
		return err;
	case DIAG_IOCTL_GET_REAL_TIME:
		err = __diag_lsm_comm_ioctl(request, buf, len, buf, len);
		break;
	case DIAG_IOCTL_CONFIG_BUFFERING_TX_MODE:
	case DIAG_IOCTL_BUFFERING_DRAIN_IMMEDIATE:
	case DIAG_IOCTL_VOTE_REAL_TIME:
	case DIAG_IOCTL_QUERY_PD_LOGGING:
	case DIAG_IOCTL_SET_OVERRIDE_PID:
	case DIAG_IOCTL_REGISTER_CALLBACK:
	case DIAG_IOCTL_MDM_HDLC_TOGGLE:
	case DIAG_IOCTL_HDLC_TOGGLE:
		err = __diag_lsm_comm_ioctl(request, NULL, 0, buf, len);
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err >= 0)
		return 0;
	else
		return err;
}

/*===========================================================================

FUNCTION diag_lsm_comm_write

DESCRIPTION
  If /dev/diag exists calls write to kernel diag driver else tries to send
  data to diag hidl server

DEPENDENCIES
   None

RETURN VALUE
  SUCCESS/FAILURE.

SIDE EFFECTS
  None.
===========================================================================*/
int diag_lsm_comm_write(int fd, unsigned char buf[], int bytes)
{
	int bytes_written = 0, status = 0;
	hidl_memory mem_s;

	if (diag_use_dev_node) {
		bytes_written = write(fd,(const void*)buf, bytes);
		return bytes_written;
	} else {
		if (ashmemAllocator == NULL)
			return -ENOMEM;
		ashmemAllocator->allocate(bytes, [&](bool success, const hidl_memory& mem) {
			if (!success) {
				DIAG_LOGE("ashmem allocate failed!!");
				status = -1;
				return;
			}
			mem_s = mem;
			sp<IMemory> memory = mapMemory(mem);
			if (memory == nullptr) {
				DIAG_LOGE("%s: Could not map HIDL memory to IMemory", __func__);
				status = -1;
				return;
			}
			void* data = memory->getPointer();
			if (data == nullptr) {
				DIAG_LOGE("%s: Could not get pointer to memory", __func__);
				status = -1;
				return;
			}
			memory->update();
			memcpy(data, buf, bytes);
			memory->commit();

		});
		if (status) {
			DIAG_LOGE("diag: In %s failed to alloc memory\n", __func__);
			return -1;
		}
		pthread_mutex_lock(&m_diag_client_mutex);
		if (!mDiagClient) {
			pthread_mutex_unlock(&m_diag_client_mutex);
			return -1;
		}
		auto ret = mDiagClient->write(mem_s, bytes);
		pthread_mutex_unlock(&m_diag_client_mutex);
		if (!ret.isOk()) {
		} else {
			bytes_written = ret;
		}

		if (bytes_written < 0) {
			return -1;
		} else {
			if (*(int *)buf == DCI_DATA_TYPE)
				return bytes_written;
			else
				return 0;
		}
	}
}

/*===========================================================================

FUNCTION diag_lsm_comm_read

DESCRIPTION
  If /dev/diag exists calls read to kernel diag driver else tries to read
  data sent by diag hidl server to client over callback.

DEPENDENCIES
   None

RETURN VALUE
  SUCCESS/FAILURE.

SIDE EFFECTS
  None.
===========================================================================*/
int diag_lsm_comm_read()
{
	int num_bytes_read = 0;

	pthread_mutex_lock(&read_data_mutex);
	if (!read_buf_busy) {
		pthread_cond_wait(&read_data_cond, &read_data_mutex);
	}
	if (diag_lsm_kill) {
		pthread_mutex_unlock(&read_data_mutex);
		DIAG_LOGE("diag: %s: exit\n", __func__);
		errno = ECANCELED;
		return -1;
	}
	if (mDiagClient == nullptr) {
		pthread_mutex_unlock(&read_data_mutex);
		DIAG_LOGE("diag: %s: HIDL interface is down\n", __func__);
		return 0;
	}

	num_bytes_read = data_len;
	if (*(int *)read_buffer == DEINIT_TYPE) {
		read_buf_busy = 0;
		pthread_mutex_unlock(&read_data_mutex);
		errno = ESHUTDOWN;
		return -1;
	}
	process_diag_payload(num_bytes_read);
	pthread_mutex_unlock(&read_data_mutex);
	pthread_mutex_lock(&write_data_mutex);
	read_buf_busy = 0;
	pthread_cond_signal(&write_data_cond);
	pthread_mutex_unlock(&write_data_mutex);

	return num_bytes_read;
}

void diag_kill_comm_read(void)
{
	pthread_mutex_lock(&read_data_mutex);
	DIAG_LOGE("%s: signalling hidl read thread exit\n", __func__);
	pthread_cond_signal(&read_data_cond);
	pthread_mutex_unlock(&read_data_mutex);

	pthread_mutex_lock(&write_data_mutex);
	DIAG_LOGE("%s: signalling hidl write thread exit\n", __func__);
	read_buf_busy = 0;
	pthread_cond_signal(&write_data_cond);
	pthread_mutex_unlock(&write_data_mutex);
}

/*===========================================================================

FUNCTION diag_process_data

DESCRIPTION
  Process the data received on callback from hidl server and signal the read
  thread to read the data.

DEPENDENCIES
   None

RETURN VALUE
  SUCCESS/FAILURE.

SIDE EFFECTS
  None.
===========================================================================*/
int diag_process_data(unsigned char *data, int len)
{

	pthread_mutex_lock(&write_data_mutex);
	if (read_buf_busy) {
		pthread_cond_wait(&write_data_cond, &write_data_mutex);
	}
	if (diag_lsm_kill) {
		pthread_mutex_unlock(&write_data_mutex);
		DIAG_LOGE("%s: exit\n", __func__);
		return 0;
	}
	memcpy(read_buffer, data, len);
	data_len = len;
	read_buf_busy = 1;
	pthread_mutex_unlock(&write_data_mutex);
	pthread_mutex_lock(&read_data_mutex);
	pthread_cond_signal(&read_data_cond);
	pthread_mutex_unlock(&read_data_mutex);

	return 0;

}

int diag_lsm_comm_active(void)
{
	if (mDiagClient)
		return 1;
	return 0;
}
