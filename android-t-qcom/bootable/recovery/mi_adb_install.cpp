/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <signal.h>

#include <cutils/properties.h>
#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/properties.h>
#include <android-base/unique_fd.h>


#include "otautil/sysutil.h"
#include "miutil/mi_system.h"
#include "recovery_ui/ui.h"
#include "recovery_ui/device.h"
#include "install/install.h"
#include "install/adb_install.h"
#include "mi_adb_install.h"
#include "fuse_sideload.h"
#include "install/mi_wipe.h"
#include "recovery_utils/logging.h"
#include "recovery.h"

//static int do_abort = 0;
#include "recovery_utils/roots.h"
#include "miutil/mi_verification.h"
#include "miutil/mi_ersfrp_verification.h"
#include "minmtp/minmtp.h"
#include "miutil/mi_utils.h"
#include "miutil/mi_serial.h"
#include "openssl/base64.h"
#include "install/wipe_data.h"


static MinMtp* minmtp = NULL;
static bool is_data_format = false;

static unsigned char *base64_mitoken = NULL;
static int base64_mitoken_length;

#define WAIT_USB_TIMEOUT_SECOND 20

static void
set_usb_function(RecoveryUI* ui, const char * property) {
    int fd= open("/sys/class/android_usb/android0/enable", O_WRONLY);
    if (fd != -1) {
        property_set("sys.usb.configfs", "0");
        if (close(fd) < 0) {
            ui->Print("failed to close driver control: %s\n", strerror(errno));
        }
    } else {
        property_set("sys.usb.configfs", "1");
    }
    property_set("sys.usb.config", property);
}

void handle_signal(int signo)
 {
    if (signo == SIGHUP) {
        printf("child recv SIGHUP..\n");
        _exit(-1);
    }
}

// close a file, log an error if the error indicator is set
static void
check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (ferror(fp)) LOG(ERROR) << "Error in " << name << "(" << strerror(errno) << ")";
    fclose(fp);
    if (chmod(name, 0644) < 0) {
        LOG(ERROR) << "chmod error: " << strerror(errno);
    }
}

static bool usb_connected(){
    int fd = open("/sys/class/android_usb/android0/state", O_RDONLY);
    if (fd < 0) {
        printf("failed to open /sys/class/android_usb/android0/state: %s\n",
               strerror(errno));
        return false;
    }

    char buf;
    /* USB is connected if android_usb state is CONNECTED or CONFIGURED */
    int connected = (read(fd, &buf, 1) == 1) && (buf == 'C');
    if (close(fd) < 0) {
        printf("failed to close /sys/class/android_usb/android0/state: %s\n",
               strerror(errno));
    }
    return connected;
}

static int  WaitComment(int pipe) {
    int maxfd = pipe +1;
    int ret;
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(pipe, &set);

    timeout.tv_sec = WAIT_COMMENT_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    ret = select(maxfd, &set, NULL, NULL, &timeout);

    if ((ret > 0) && (!FD_ISSET(pipe, &set))) return 0;
    return ret;
}

static void format_data(RecoveryUI* ui, FILE *to_child)
 {
    int status = 0;
    if (disable_format()) {
        fprintf(to_child, "Format_data_is_already_disable.\n");
        return;
    }
    status = ui->SubMenuWipeCmdFormatData();
    if (status != INSTALL_SUCCESS) {
        fprintf(to_child, "Format_data_failed.\n");
    } else {
        is_data_format = true;
        fprintf(to_child, "Format_data_success.\n");
    }
}

static void format_cache(RecoveryUI* ui, FILE *to_child)
 {
    int status = 0;
    if (disable_format()) {
        fprintf(to_child, "Format_data_is_already_disable.\n");
        return;
    }
    status = ui->SubMenuWipeCmdWipeCache();
    if (status != INSTALL_SUCCESS) {
        fprintf(to_child, "Format_cache_failed.\n");
    } else {
        fprintf(to_child, "Format_cache_success.\n");
    }
}

static void format_storage(RecoveryUI* ui, FILE *to_child)
{
    int status = 0;
    if (disable_format()) {
        fprintf(to_child, "Format_data_is_already_disable.\n");
        return;
    }
    status = ui->SubMenuWipeCmdFormatStorage();
    if (status != INSTALL_SUCCESS) {
        fprintf(to_child, "Format_storage_failed.\n");
    } else {
        fprintf(to_child, "Format_storage_success.\n");
    }
}

static void format_data_and_storage(RecoveryUI* ui, FILE *to_child)
{
    int status = 0;
    if (disable_format()) {
        fprintf(to_child, "Format_data_is_already_disable.\n");
        return;
    }
    status  = ui->SubMenuWipeCmdFormatData();
    status |= ui->SubMenuWipeCmdFormatStorage();
    if (status != INSTALL_SUCCESS) {
        fprintf(to_child, "Format_data_and_storage_failed.\n");
    } else {
        fprintf(to_child, "Format_data_and_storage_success.\n");
    }
}

static void wipe_data_and_storage(RecoveryUI* ui, FILE *to_child)
{
    int status = 0;
    if (disable_format()) {
        fprintf(to_child, "Format_data_is_already_disable.\n");
        return;
    }
    status  = ui->SubMenuWipeCmdWipeData(false);
    status |= ui->SubMenuWipeCmdFormatStorage();
    if (status != INSTALL_SUCCESS) {
        fprintf(to_child, "Wipe_data_and_storage_failed.\n");
    } else {
        fprintf(to_child, "Wipe_data_and_storage_success.\n");
    }
}

static inline void free_mitoken() {
    if(!base64_mitoken)
        free(base64_mitoken);
    base64_mitoken = NULL;
    base64_mitoken_length = 0;
    return;
}

/**
 * @brief the mitoken's format and each data's offset show below
 *
 *  mitoken-no:0, type: random
 *   | type(1B) | len(1B) | random_value0(8B) | random_value1(8B) | => 18B
 *  off:4
 *
 *  mitoken-no:1, type: cpuid
 *   | type(1B) | len(1B) | cpuid_value(mB) |   => (2 + m)B
 *  off:22               off:24
 *
 *  mitoken-no:2, type: product
 *   | type(1B) | len(1B) | product_value(nB) | => (2 + n)B
 *  off:(24 + m)         off:(26 + m)
 *
 *  protocol_message:
 *   | HEAD(1B) | VERSION(1B) | VENDOR(1B) | LEN(1B) | MITOKENS(22 + n)B | => (20 + n + m)B
 *  off:0      off:1         off:2        off:3     off:4
 *
 *  the final length does not include the ending '\0'
 *
 */
static void transport_ersfrp_verity_message(FILE *to_child) {

#define LOGT(format, args...) \
    { fprintf(to_child, format, ##args); printf(format, ##args); }

    // avoid using `adb getmitoken` more than once
    free_mitoken();

    char *buffer = (char *) malloc(PROPERTY_VALUE_MAX * 2);
    if (buffer == NULL) {
        LOGT("transport_ersfrp_verity_message malloc blk fail\n");
        return;
    }
    // get vendor_value and its length
    memset(buffer, '\0', PROPERTY_VALUE_MAX * 2);
    if(!property_get("ro.hardware", buffer, NULL)) {
        LOGT("failed_to_get_hardware\n");
        free(buffer);
        return;
    }
    char *hardware_off = buffer;
    int hardware_length = strlen(buffer);
    printf("success_to_get_vendor:%s\n", buffer);

    // get product_value's length, which is the variable 'n' in
    // the comment
    buffer += hardware_length;
    if(!property_get("ro.build.product", buffer, NULL)) {
        LOGT("failed_to_get_product\n");
        free(buffer);
        return;
    }
    char *product_off = hardware_off + hardware_length;
    int product_length = strlen(buffer);
    printf("success_to_get_product:%s\n", buffer);

    // get cpuid_value's length, which is the variable 'm' in
    // the comment
    buffer += product_length;
    if(!strncmp(buffer - product_length, "omega", 5) ||
            !strncmp(buffer - product_length, "nikel", 5)) {
        if(get_longcheer_sn(buffer) < 0) {
            LOGT("failed_to_get_cpuid\n");
        }
    } else {
        if(get_miphone_sn(buffer) < 0) {
            LOGT("failed_to_get_cpuid\n");
        }
    }
    char *cpuid_off = product_off + product_length;
    int cpuid_length = strlen(buffer);
    printf("success_to_get_cpuid:%s\n", buffer);

    // generate first random number
    srandom(((unsigned) time(NULL)));
    buffer += cpuid_length;
    sprintf(buffer, "%08lx", random());
    char *random0_off = cpuid_off + cpuid_length;
    printf("success_to_get_random0:%s\n", buffer);

    // generate second random number
    buffer += 8;
    sprintf(buffer, "%08lx", random());
    char *random1_off = random0_off + 8;
    printf("success_to_get_random1:%s\n", buffer);

    int mitoken_length = 2 * 3 + 16 + (cpuid_length - 2/*0x*/) / 2 + product_length + 4;
    printf("mitoken_length:%d\n", mitoken_length);

    unsigned char mitoken[mitoken_length];
    memset(mitoken, '\0', mitoken_length);

    // inflate the protocol message's metadata
    int off = 0;
    mitoken[off] = 0x55;
    mitoken[off + 1] = 0x01;
    if(!strncmp("qcom", hardware_off, hardware_length)) {
        mitoken[off + 2] = 0x01;
    } else if (!strncmp("mt", hardware_off, 2) || !strncmp("MT", hardware_off, 2)) {
        mitoken[off + 2] = 0x02;
    } else if (!strncmp("meri", hardware_off, hardware_length)) {
        mitoken[off + 2] = 0x03;
    } else {
        LOGT("failed_to_get_hardware\n");
        free(buffer);
        return;
    }
    mitoken[off + 3] = mitoken_length - 4;

    // inflate the random value
    off += 4;
    mitoken[off] = 0x01;
    mitoken[off + 1] = 16;
    memcpy(mitoken + off + 2, random0_off, 8);
    memcpy(mitoken + off + 10, random1_off, 8);

    // inflate the cpuid value
    off += 18;
    mitoken[off] = 0x02;
    mitoken[off + 1] = (cpuid_length - 2/*0x*/) / 2;
    cpuid_off += 2;
    for(int i = 0; i < mitoken[off + 1]; i ++) {
        if(*cpuid_off >= '0' && *cpuid_off <= '9') {
            mitoken[off + 2 + i] |= ((*cpuid_off - '0') << 4);
        } else if (*cpuid_off >= 'a' && *cpuid_off <= 'f') {
            mitoken[off + 2 + i] |= ((*cpuid_off - 'a' + 10) << 4);
        } else {
            mitoken[off + 2 + i] |= ((*cpuid_off - 'A' + 10) << 4);
        }

        cpuid_off ++;
        if(*cpuid_off >= '0' && *cpuid_off <= '9') {
            mitoken[off + 2 + i] |= (*cpuid_off - '0');
        } else if (*cpuid_off >= 'a' && *cpuid_off <= 'f') {
            mitoken[off + 2 + i] |= (*cpuid_off - 'a' + 10);
        } else {
            mitoken[off + 2 + i] |= (*cpuid_off - 'A' + 10);
        }
        cpuid_off ++;
    }

    // inflate the product value
    off += (2 + (cpuid_length - 2/*0x*/) / 2);
    mitoken[off] = 0x03;
    mitoken[off + 1] = product_length;
    memcpy(mitoken + off + 2, product_off, product_length);

    // transfer the protocol_message to base64 data
    size_t out_len;
    EVP_EncodedLength(&out_len, mitoken_length);

    // according to the defination of EVP_EncodedLength, the
    // out_len include the final '\0', but for the MIFlash tools,
    // the final length of base64_mitoken is unknown, so allocate
    // another byte to store the length of final base64_mitoken.
    //
    // the final base64_mitoken's format:
    // | base64_mitoken_len(1B) | base64_mitoken(xB) |
    base64_mitoken = (unsigned char *) malloc(out_len + 1);
    if (base64_mitoken == NULL) {
        free(buffer);
        printf("base64_mitoken malloc blk fail\n");
        return;
    }
    base64_mitoken[0] = (base64_mitoken_length = out_len - 1);
    if(EVP_EncodeBlock(base64_mitoken + 1, mitoken, mitoken_length) != out_len - 1) {
        LOGT("failed_to_get_base64_length\n");
        free_mitoken();
    } else {
        // client receive base64_mitoken
        LOGT("%s\n", base64_mitoken);
    }

    free(buffer);
#undef LOGT
}

static void format_frp(RecoveryUI* ui, FILE *to_child, const unsigned char* signature)
{
#define LOGT(format, args...) \
    { fprintf(to_child, format, ##args); printf(format, ##args); }

    //1. verify the signature
    if(base64_mitoken == NULL) {
        LOGT("failed_mitoken_is_NULL\n");
        return;
    } else {
        unsigned char *sig = (unsigned char *) malloc(RSANUMBYTES + 1);
        if (sig == NULL) {
            free_mitoken();
            LOGT("sig malloc blk fail\n");
            return;
        }
        memset(sig, '\0', RSANUMBYTES + 1);
        int i, j;
        for(i = 0, j = 0; i < RSANUMBYTES * 2; i += 2, j ++) {
            unsigned char highbyte, lowbyte;
            if(signature[i] >= '0' && signature[i] <= '9') {
                highbyte = (signature[i] - '0') << 4;
            } else {
                highbyte = (signature[i] - 'A' + 10) << 4;
            }

            if(signature[i + 1] >= '0' && signature[i + 1] <= '9') {
                lowbyte = signature[i + 1] - '0';
            } else {
                lowbyte = signature[i + 1] - 'A' + 10;
            }

            sig[j] = highbyte | lowbyte;
        }
        if(i != RSANUMBYTES * 2) {
            LOGT("failed_siglen_is_not_512\n");
            free_mitoken();
            free(sig);
            return;
        } else {
            int result = rsa_decrypt(sig, RSANUMBYTES, base64_mitoken + 1, base64_mitoken_length);
            if(result == CRYPT_OK) {
                printf("success_decrypt_ok\n");
            } else {
                LOGT("failed_decrypt_error\n");
                free_mitoken();
                free(sig);
                return;
            }
        }
        free(sig);
    }

    // 2. erase data partition
    if(!EraseVolume("/data", ui)) {
        LOGT("erase_data_failed\n");
        free_mitoken(); return;
    }

    // 3. erase frp partition
    char property_buffer[PROPERTY_VALUE_MAX * 2] = { 0 };
    if(!property_get("ro.frp.pst", property_buffer, NULL)){
        LOGT("failed_to_get_frp_name\n");
        free_mitoken(); return;
    }

    android::base::unique_fd fd(open(property_buffer, O_WRONLY));
    if (fd == -1) {
        LOGT("failed_to_open_%s:%s\n", property_buffer, strerror(errno));
        free_mitoken(); return;
    }

    off_t file_length = -1;
    if ((file_length = lseek(fd, 0, SEEK_END)) == -1) {
        LOGT("failed_to_lseek_%s:%s\n", property_buffer, strerror(errno));
        free_mitoken(); return;
    }

    lseek(fd, 0, SEEK_SET);
    char *p = (char *) malloc(file_length);
    if (p == NULL) {
        printf("p malloc blk fail\n");
        free_mitoken(); return;
    }
    memset(p, '\0', file_length);
    if (!android::base::WriteFully(fd, p, file_length)) {
        LOGT("failed_to_write_%s:%s\n", property_buffer, strerror(errno));
        free(p);
        free_mitoken(); return;
    }

    if (fsync(fd) == -1) {
        LOGT("failed_to_fsync_%s:%s\n", property_buffer, strerror(errno));
        free(p);
        free_mitoken(); return;
    }
    free(p);
    fprintf(to_child, "success_to_erase_frp_and_data\n");
#undef LOGT
}

static void is_enabled(FILE *to_child)
{
    if(minmtp && minmtp->is_enable()) {
        fprintf(to_child, "Mtp_is_enabled\n");
    } else {
        fprintf(to_child, "Mtp_is_disabled\n");
    }
}

static void enable_mtp(RecoveryUI* ui, FILE *to_child)
{
    const char * path = "/data";
    // Foramt "/data" partition to store install-package.
    // Install-package maybe too large to store in other partition, but the "/data" partition
    // is encrypted, so we have to format "/data" partition to mount.

    // if (!is_data_format) {
    //     erase_volume(path);
    //     is_data_format = true;
    // }

    if (ensure_path_mounted(path) < 0) {
        LOG(ERROR) << "format data partition failed.";
        fprintf(to_child, "Format_data_failed.\n");
        return;
    }

    // enable mtp
    if (!minmtp) {
        minmtp = new MinMtp();
        //1.set usb config to "mtp,adb"
        char usb_state[PROPERTY_VALUE_MAX + 1] = {0};
        set_usb_function(ui, "mtp,adb");

        //2.wait for usb config ready
        for (int j = 0; j < WAIT_USB_TIMEOUT_SECOND; j++){
            property_get("sys.usb.state", usb_state, "none");
            LOG(ERROR) << "usb state :" << usb_state;
            if(strcmp(usb_state, "mtp,adb") == 0)
                break;
            sleep(1);
        }

        //3.enable mtp
        if (strcmp(usb_state, "mtp,adb") == 0){
            if (minmtp->enable_mtp(path)) {
                fprintf(to_child, "Enabled_mtp_success.\n");
                return;
            }
        }
        //4.if failed reset minmtp and usb
        LOG(ERROR) << "Enable mtp failed.";
        fprintf(to_child, "Enable_mtp_failed.\n");
        delete minmtp;
        minmtp = NULL;
        set_usb_function(ui, "adb");
    } else {
        fprintf(to_child, "Mtp_is_already_enabled.\n");
    }
}

static void disable_mtp(RecoveryUI* ui, FILE *to_child)
{
    if (minmtp && minmtp->is_enable()){
        set_usb_function(ui, "adb");
        if (!minmtp->disable_mtp()) {
            LOG(ERROR) << "Disable mtp failed.";
            fprintf(to_child, "Disable_mtp_failed.\n");
            return;
        }
        delete minmtp;
        minmtp = NULL;
        fprintf(to_child, "Disabled_mtp_success.\n");
    } else {
        fprintf(to_child, "Mtp_is_already_disabled.\n");
    }
}

static void mtp_install(RecoveryUI* ui, FILE *to_child, char * verification)
{
    static const char *TEMPORARY_INSTALL_FILE = "/tmp/last_install";
    int install_result = INSTALL_CORRUPT;
    if (!is_data_format) {
        LOG(ERROR) << "Data is not format for mtp-install.";
        fprintf(to_child, "Mtp_install_failed.\n");
        return;
    }

    if (verification == NULL) {
        LOG(ERROR) << "verification is NULL.";
        fprintf(to_child, "Mtp_install_failed.\n");
        return;
    }

    if(minmtp){
        //1.get install package path from mtp database.
        const char * install_file = minmtp->get_file_path();
        if (install_file == NULL) {
            LOG(ERROR) << "install_file is NULL.";
            fprintf(to_child, "Mtp_install_failed.\n");
            return;
        }

        LOG(ERROR) << "mtp_install path:" << install_file;
        LOG(ERROR) << "verification message:" << verification;

        //2.verify message check.
        if ( verification) {
            bool wipe_cache = false;
            install_result = ui->StartInstallPackage(install_file, &wipe_cache,
                    TEMPORARY_INSTALL_FILE, true, verification);
        } else {
            LOG(ERROR) << "Install verification failed.";
            install_result = INSTALL_VERIFY_FAILURE;
            fprintf(to_child, "Installation_verify_failed.\n");
            save_install_status(install_result);
            return;
        }
    } else {
        LOG(ERROR) << "Mtp is not ready.";
        install_result = INSTALL_ERROR;
        fprintf(to_child, "Mtp_install_error.\n");
        save_install_status(install_result);
        return;
    }

    save_install_status(install_result);
    ui->SetBackground(RecoveryUI::NO_COMMAND);
    if (install_result != INSTALL_SUCCESS) {
        LOG(ERROR) << "Installation aborted.";
        fprintf(to_child, "Installation_aborted.\n");
        return;
    } else {
        LOG(ERROR) << "Install from MTP complete.";
        fprintf(to_child, "Installation_complete.\n");
    }
}

void save_install_status(int new_status) {
    const char* status= NULL;
    switch (new_status) {
        case INSTALL_SUCCESS:
        status = "INSTALL_SUCCESS";
        break;
        case INSTALL_NONE:
        status = "INSTALL_NONE";
        break;
        case INSTALL_NO_FILE:
        status = "INSTALL_NO_FILE";
        break;
        case INSTALL_VERIFY_FAILURE:
        status = "INSTALL_VERIFY_FAILURE";
        break;
        case INSTALL_ERROR:
        status = "INSTALL_ERROR";
        break;
        case INSTALL_CORRUPT:
        status = "INSTALL_CORRUPT";
        break;
    }
    if (status != NULL) {
        LOG(INFO) << "Saving new_status:" << status;
        FILE* fp = fopen_path(STATUS_FILE, "w");
        if (fp == NULL) {
            LOG(ERROR) << "Can't open" << STATUS_FILE << ":" << strerror(errno);
        } else {
            fwrite(status, 1, strlen(status), fp);
            fsync(fileno(fp));
            check_and_fclose(fp, STATUS_FILE);
        }
    }
}

static void wipe_efs(RecoveryUI* ui, FILE *to_child)
{
    int status = 0;
    status = ui->SubMenuWipeCmdWipeEfs();
    if (status != INSTALL_SUCCESS) {
        fprintf(to_child, "wipe_efs_failed.\n");
    } else {
        fprintf(to_child, "wipe_efs_success.\n");
    }
}

void stop_adbd(RecoveryUI* ui) {
  ui->Print("Stopping adbd...\n");
  android::base::SetProperty("ctl.stop", "adbd");
  android::base::SetProperty("sys.usb.config_disable", "1");
  set_usb_driver(false);
}

void set_usb_driver(bool enabled) {
  // USB configfs doesn't use /s/c/a/a/enable.
  if (android::base::GetBoolProperty("sys.usb.configfs", false)) {
    return;
  }

  static constexpr const char* USB_DRIVER_CONTROL = "/sys/class/android_usb/android0/enable";
  android::base::unique_fd fd(open(USB_DRIVER_CONTROL, O_WRONLY));
  if (fd == -1) {
    PLOG(ERROR) << "Failed to open driver control";
    return;
  }
  // Not using android::base::WriteStringToFile since that will open with O_CREAT and give EPERM
  // when USB_DRIVER_CONTROL doesn't exist. When it gives EPERM, we don't know whether that's due
  // to non-existent USB_DRIVER_CONTROL or indeed a permission issue.
  if (!android::base::WriteStringToFd(enabled ? "1" : "0", fd)) {
    PLOG(ERROR) << "Failed to set driver control";
  }
}

Device::BuiltinAction
prompt_and_wait_adb_install(RecoveryUI* ui, const char* install_file)
{
    stop_adbd(ui);
    set_usb_function(ui, "adb");

    LOG(ERROR) << "Adb start sideload <filename> " << install_file;
    int pipe_in[2], pipe_out[2];
    pipe(pipe_in);
    pipe(pipe_out);

    pid_t child;
    if ((child = fork()) == 0) {
        const char *binary = "/system/bin/recovery";
        char* args[5];
        char in[10], out[10];
        args[0] = (char *)binary;
        args[1] = (char *)"--adbd";
        args[2] = in;
        args[3] = out;
        args[4] = NULL;
        sprintf(in, "%d", pipe_in[1]);
        sprintf(out, "%d", pipe_out[0]);
        close(pipe_in[0]);
        close(pipe_out[1]);
        signal(SIGHUP, handle_signal);
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        execv(binary, args);
        LOG(ERROR) << "Can't run " << binary << ":" << strerror(errno);
        _exit(-1);
    }
    close(pipe_in[1]);
    close(pipe_out[0]);


    int status;
    int read_state;
    bool waited = false;
    int install_result;
    Device::BuiltinAction result = Device::NO_ACTION;
    struct stat st;

    FILE *to_child = fdopen(pipe_out[1], "wb");
    setlinebuf(to_child);
    char buffer[1024];
    FILE* from_child = fdopen(pipe_in[0], "r");
    for(;;) {
        read_state = WaitComment(pipe_in[0]);
        if (read_state == 0 && usb_connected()) {
                continue;
        } else if(read_state <= 0) {
            if (read_state < 0) LOG(ERROR) << "Error read_state.";
            LOG(ERROR) << "Time out after WAIT_COMMENT_TIMEOUT_SEC, unless a USB cable is  plugged in.";
            set_reboot_message(0);
            sync();
            result = Device::REBOOT;
            break;
        }
        if(fgets(buffer, sizeof(buffer), from_child) == NULL) break;
        char* command = strtok(buffer, " \n");
        if (command == NULL) {
            printf("Commond is NULL.\n");
            continue;
        } else if (strncmp(command, "install_package:",16) == 0) {
            LOG(ERROR) << "We will install package scan file";
            char* saveptr;
            char* s = strtok_r(command, ":", &saveptr);
            char* ciphertext = strtok_r(NULL, ":", &saveptr);
            s = strtok_r(NULL, ":", &saveptr);
            int is_reboot = atoi(s);
            printf("Verifier msg :%s\nreboot : %d\n", ciphertext, is_reboot);
            for (int i = 0; i < ADB_INSTALL_TIMEOUT; ++i) {
                if (waitpid(child, &status, WNOHANG) != 0) {
                    install_result = INSTALL_ERROR;
                    waited = true;
                    LOG(ERROR) << "Install package failed child aborted";
                    break;
                }

                if (stat(FUSE_SIDELOAD_HOST_PATHNAME, &st) != 0) {
                    if (errno == ENOENT && i < ADB_INSTALL_TIMEOUT-1) {
                        sleep(1);
                        LOG(ERROR) << "Install package is scaning";
                        continue;
                    } else {
                        LOG(ERROR) << "Timed out waiting for package.";
                        install_result = INSTALL_ERROR;
                        kill(child, SIGKILL);
                        break;
                    }
                }
                LOG(ERROR) << "Start install package.";
                bool wipe_cache = false;
                install_result = ui->StartInstallPackage(FUSE_SIDELOAD_HOST_PATHNAME, &wipe_cache, install_file, false, ciphertext, true);
                break;
            }

            if (!waited) {
                stat(FUSE_SIDELOAD_HOST_EXIT_PATHNAME, &st);
            }

            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                if (WEXITSTATUS(status) == 3) {
                    LOG(ERROR) << "You need adb 1.0.32 or newer to sideload to this device.";
                } else if (!WIFSIGNALED(status)) {
                    LOG(ERROR) << "(Adbd status " << WEXITSTATUS(status) << ")";
                }
            }
            if (install_result >= 0) {
                save_install_status(install_result);
                if (install_result != INSTALL_SUCCESS) {
                    ui->SetBackground(RecoveryUI::NO_COMMAND);
                    LOG(ERROR) << "Installation aborted.";
                    fprintf(to_child, "Installation_aborted.\n");
                } else {
                    LOG(ERROR) << "Install from ADB complete.";
                    fprintf(to_child, "Installation_complete.\n");
                    if (is_reboot) {
                        sleep(3);
                        set_reboot_message(0);
                        sync();
                        result = Device::REBOOT;
                        kill(child, SIGKILL);
                        break;
                    }
                    ui->SetBackground(RecoveryUI::NO_COMMAND);
                }
            }
        } else if (strcmp(command, "format_data") == 0) {
            LOG(ERROR) << "format_data start";
            format_data(ui, to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "format_cache") == 0) {
            LOG(ERROR) << "format_cache start";
            format_cache(ui, to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "format_storage") == 0) {
            LOG(ERROR) << "format_storage start";
            format_storage(ui, to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "format_data_and_storage") == 0) {
            LOG(ERROR) << "format_data_and_storage satrt";
            format_data_and_storage(ui, to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "wipe_data_and_storage") == 0) {
            LOG(ERROR) << "wipe_data_and_storage start";
            wipe_data_and_storage(ui, to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "reboot") == 0) {
            LOG(ERROR) << "reboot: start";
            set_reboot_message(0);
            sync();
            fprintf(to_child, "reboot.\n");
            result = Device::REBOOT;
            // zhangliguang add log
            ensure_path_unmounted("/cache");
            break;
        } else if (strcmp(command, "rebootbootloader") == 0) {
            LOG(ERROR) << "reboot:bootloader start";
            fprintf(to_child, "reboot-bootloader.\n");
            result = Device::REBOOT_BOOTLOADER;
            break;
        } else if (strcmp(command, "rebootfastboot") == 0) {
            LOG(ERROR) << "reboot:fastboot start";
            fprintf(to_child, "reboot-fastboot.\n");
            result = Device::ENTER_FASTBOOT;
            break;
        } else if (strcmp(command, "rebootrecovery") == 0) {
            LOG(ERROR) << "reboot:recovery start";
            fprintf(to_child, "reboot-recovery.\n");
            result = Device::REBOOT_RECOVERY;
            break;
        } else if (strcmp(command, "shutdown") == 0) {
            LOG(ERROR) << "shutdown: start";
            fprintf(to_child, "shutdown.\n");
            result = Device::SHUTDOWN;
            break;
        } else if (strcmp(command, "wipe_efs") == 0) {
            LOG(ERROR) << "wipe_efs start";
            wipe_efs(ui, to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "enablemtp") == 0) {
            LOG(INFO) << "enable mtp start";
            enable_mtp(ui, to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "disablemtp") == 0) {
            LOG(INFO) << "disable mtp start";
            disable_mtp(ui, to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strncmp(command, "mtpinstall:", 11) == 0) {
            LOG(INFO) << "mtp install start";
            char * verification = command + strlen("mtpinstall:");
            mtp_install(ui, to_child, verification);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "isenable") == 0) {
            LOG(INFO) << "judge whether the mtp is enabled";
            is_enabled(to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strncmp(command, "format-frp:", 11) == 0) {
            LOG(INFO) << "format the partition frp:" << command << "\n";
            char *signature;
            strtok_r(command, ":", &signature);
            format_frp(ui, to_child, (unsigned char *)signature);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else if (strcmp(command, "getmitoken") == 0) {
            LOG(INFO) << "get mitoken";
            transport_ersfrp_verity_message(to_child);
            ui->SetBackground(RecoveryUI::NO_COMMAND);
        } else {
            fprintf(to_child, "%s\n", command);
            LOG(INFO) << "unknown command [" << command << "]";
        }
    }
    fclose(to_child);
    fclose(from_child);

    return result;
}
