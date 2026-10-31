#ifndef ANDROID_VOLD_IMPL_H
#define ANDROID_VOLD_IMPL_H

#include <log/log.h>
#include <cryptfs_hw.h>
#include <cryptfs.h>
#include <android-base/properties.h>
#include <cutils/properties.h>
#include <sys/param.h>
#include <libdm/dm.h>
#include <linux/dm-ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <Utils.h>
#include <selinux/android.h>
#include <EncryptInplace.h>
#include <f2fs_sparseblock.h>
#include <fs_mgr.h>
#include <wakelock/wakelock.h>
#include <bootloader_message/bootloader_message.h>
#include <VoldUtil.h>
#include <sys/mount.h>
#include <Checkpoint.h>
#include <fstab/fstab.h>
#include <log/log.h>
#include <android-base/logging.h>
#include <utils/Errors.h>
//#include <Keymaster.h>
#include <IVoldStub.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <KeyUtil.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>

/* For T bringup
extern "C" {
#include <crypto_scrypt.h>
}

#define DEFAULT_HEX_PASSWORD "64656661756c745f70617373776f7264"
#define CRYPT_ASCII_PASSWORD_UPDATED 0x1000
#define KEY_LEN_BYTES 16
#define DATA_MNT_POINT "/data"
#define METADATA_MNT_POINT "/metadata"
#define DM_CRYPT_BUF_SIZE 4096
#define RETRY_MOUNT_ATTEMPTS 10
#define RETRY_MOUNT_DELAY_SECONDS 1
*/
namespace android {
namespace vold {
class VoldImpl : public IVoldStub {
public:
        virtual ~VoldImpl() {}
        /* for T bring up , function was deleted on .cpp; need developer adaptation
        virtual int cryptfs_changepw_hw_fde(int crypt_type, const char *currentpw,
                                            const char *newpw);
        virtual int cryptfs_check_passwd_hw(const char *passwd);
        virtual int cryptfs_enable_internal_hw(int crypt_type, const char* passwd, int no_ui);
        virtual int cryptfs_restart_internal_hw(int restart_main);
        virtual int verify_hw_fde_passwd(const char *passwd, struct crypt_mnt_ftr* crypt_ftr);
        virtual bool is_metadata_wrapped_key_supported();

        virtual status_t WaitDmReady(std::string& mDmDevPath);
        //virtual bool KeyUpdate(km::ErrorCode code, const KeyBuffer& kmKey, Keymaster& keymaster, std::string& key_temp);
        virtual bool SetFbeIceKey(km::AuthorizationSet& paramBuilder);


        // add for fbe/me key backup and restore
        virtual bool retrieveOrGenerateKeyStub(const std::string& key_path, const std::string& tmp_path,
              const KeyAuthentication& key_authentication, const KeyGeneration& gen,
              KeyBuffer* key, const std::string& key_back_path);
        virtual bool retrieveKeyStub(const std::string& dir, const KeyAuthentication& auth, KeyBuffer* key, const std::string& key_back_path);
        virtual void backupKeyStub(const std::string& key_path, const std::string& key_back_path, bool force);
        virtual bool retrieveBackupKeyStub(const std::string& key_path, const std::string& backup_key_path,
                       const KeyAuthentication& key_authentication, KeyBuffer* key);
	virtual void SetCldListener(const android::sp<android::os::IVoldTaskListener>& listener);
	virtual int GetCldFragLevel();
	virtual int RunCldOnHal();
	*/
        virtual void SetCldListener(const android::sp<android::os::IVoldTaskListener>& listener);
	virtual int GetCldFragLevel();
	virtual int RunCldOnHal();
        virtual int runExtMFlush(int pageCount, int flushLevel, const android::sp<android::os::IVoldTaskListener>& listener);
        virtual int stopExtMFlush(const android::sp<android::os::IVoldTaskListener>& listener);

        virtual void MoveStorage(const std::string& from, const std::string& to,
                         const android::sp<android::os::IVoldTaskListener>& listener, const char *wake_lock, int flag);
        virtual void fixupAppDirRecursiveNative(const std::string& path, int32_t appUid, int32_t gid, int32_t flag);
        virtual void moveStorageQuickly(const std::string& from, const std::string& to, const android::sp<android::os::IVoldTaskListener>& listener,
                    int32_t flag, int32_t* _aidl_return);
        virtual int fixupAppDirRecursive(const std::string& path, int32_t appUid, int32_t gid, int32_t flag);

private:
/* for T bring u
        void convert_key_to_hex_ascii_for_upgrade(const unsigned char *master_key,
                                                  unsigned int keysize, char *master_key_ascii);
        int get_keymaster_hw_fde_passwd(const char* passwd, unsigned char* newpw,
                                        unsigned char* salt,
                                        const struct crypt_mnt_ftr *ftr);
        int verify_and_update_hw_fde_passwd(const char *passwd,
                                           struct crypt_mnt_ftr* crypt_ftr);
#if !defined(CONFIG_HW_DISK_ENCRYPT_PERF)
        void ioctl_init(struct dm_ioctl* io, size_t dataSize, const char* name, unsigned flags);
        int load_crypto_mapping_table(struct crypt_mnt_ftr* crypt_ftr,
                                     const unsigned char* master_key, const char* real_blk_name,
                                     const char* name, int fd, const char* extra_params);
        int create_crypto_blk_dev_hw(struct crypt_mnt_ftr* crypt_ftr, const unsigned char* master_key,
                                    const char* real_blk_name, char* crypto_blk_name, const char* name);
#endif
        int test_mount_hw_encrypted_fs(struct crypt_mnt_ftr* crypt_ftr,
                                      const char *passwd, const char *label);
        int cryptfs_get_master_key(struct crypt_mnt_ftr* ftr, const char* password,
                                  unsigned char* master_key);
        int cryptfs_create_default_ftr(struct crypt_mnt_ftr* crypt_ftr, __attribute__((unused))int key_length);
        status_t execMv(const std::string& from, const std::string& to, int startProgress,
                               int stepProgress,
                               const android::sp<android::os::IVoldTaskListener>& listener, int flag );
        status_t moveStorageQ(const std::string& from, const std::string& to,
                                            const android::sp<android::os::IVoldTaskListener>& listener, int flag);
        void notifyProgress(int progress, const android::sp<android::os::IVoldTaskListener>& listener);
        bool pushBackContents(const std::string& path, std::vector<std::string>& cmd, int searchLevels);
        status_t execRm(const std::string& path, int startProgress, int stepProgress, const android::sp<android::os::IVoldTaskListener>& listener);
 */
        void gcBoosterControl(const std::string& enable);
        bool ShouldAbort();
        void RecordTrimStart();
        void RecordTrimFinish();
        void StopTrimThread();
        void RunUrgentGcIfNeeded(const std::list<std::string>& paths);

        status_t execMv(const std::string& from, const std::string& to, int startProgress,
                                       int stepProgress,
                                       const android::sp<android::os::IVoldTaskListener>& listener, int flag );
        status_t moveStorageQ(const std::string& from, const std::string& to,
                                                    const android::sp<android::os::IVoldTaskListener>& listener, int flag);
        void notifyProgress(int progress, const android::sp<android::os::IVoldTaskListener>& listener);
        bool pushBackContents(const std::string& path, std::vector<std::string>& cmd, int searchLevels);
        status_t execRm(const std::string& path, int startProgress, int stepProgress, const android::sp<android::os::IVoldTaskListener>& listener);
};


extern "C" IVoldStub* create();
extern "C" void destroy(IVoldStub* impl);

}//namespace vold
}//namespace android

#endif
