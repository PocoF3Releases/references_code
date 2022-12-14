#define LOG_TAG "MIUINDBG_P"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <poll.h>
#include <sys/inotify.h>
#include <private/android_filesystem_config.h>
#include <cutils/properties.h>
#include <selinux/selinux.h>
#include <openssl/aes.h>
#include "ndbglog.h"


#define COREDUMP_PROP        "sys.miui.ndcd"
#define LOCAL_DEBUG_PROP     "persist.sys.miui.ndld"

__attribute__ ((weak))
int8_t property_get_bool(const char *key, int8_t default_value) {
    if (!key) {
        return default_value;
    }

    int8_t result = default_value;
    char buf[PROPERTY_VALUE_MAX] = {'\0',};

    int len = property_get(key, buf, "");
    if (len == 1) {
        char ch = buf[0];
        if (ch == '0' || ch == 'n') {
            result = false;
        } else if (ch == '1' || ch == 'y') {
            result = true;
        }
    } else if (len > 1) {
         if (!strcmp(buf, "no") || !strcmp(buf, "false") || !strcmp(buf, "off")) {
            result = false;
        } else if (!strcmp(buf, "yes") || !strcmp(buf, "true") || !strcmp(buf, "on")) {
            result = true;
        }
    }

    return result;
}

static void upload_zip()
{
#ifndef NDBG_LOCAL
    if(!property_get_bool(LOCAL_DEBUG_PROP, false)) {
        if(!setcon("u:r:adbd:s0")) {
            system("am broadcast --user 0 -a miui.bugreport.intent.action.ALARM_WAKEUP_REPORTER");
        } else {
            MILOGE("setcon failed!");
        }
    }
#endif
}

static void disable_coredump() {
#ifndef NDBG_LOCAL
    property_set(COREDUMP_PROP, "off");
#endif
}

static bool is_coredump_enable() {
   return property_get_bool(COREDUMP_PROP, false);
}

static void wait_other(char* dir)
{
    int ifd = inotify_init();
    if(ifd < 0) return;

    int wd = inotify_add_watch(ifd, dir, IN_MODIFY |IN_CLOSE_WRITE);
    if(wd < 0) return;

    char event_buf[512];

    while (1) {
        int num_bytes = 0;

        struct pollfd pfd = {ifd, POLLIN, 0 };

        int ret = poll(&pfd, 1, 1000);

        if (ret < 0) {
            MILOGD("POLL ERR!!!");
            break;
        } else if (ret == 0) {
            MILOGD("TIME OUT!!!");
            break;
        } else {
            num_bytes = read(ifd, event_buf, sizeof(event_buf));
        }
    }

    inotify_rm_watch(ifd,wd);
}


static void wait_core(char* logs_dir)
{
#ifndef NDBG_LOCAL
    if(!is_coredump_enable()) {
        return;
    }
#endif
    char core_dir[PATH_MAX];

    strcpy(core_dir,logs_dir);
    *(char*)strrchr(core_dir,'/') = '\0';
    strcat(core_dir,"/core/");
#if NDBG_LOCAL
    if (access(core_dir,F_OK))
        return;
#endif

    DIR *d;
    struct dirent *de;
    char* core_file = NULL;
    int retry_count = 3;

    do {
        d = opendir(core_dir);
        if(!d) break;

        while ((de = readdir(d)) != 0) {
            if(strncmp(de->d_name,"core-",4) == 0) {
                core_file = strdup(de->d_name);
                retry_count = 0;
                break;
            }
        }
        closedir(d);

        if(--retry_count <= 0) {
            break;
        }
        sleep(1);

    } while (1);

    if(core_file == NULL) {
       MILOGD("No Core!!!");
       return;
    }

    char proc[PATH_MAX];
    struct stat st;

    char* core_path = core_dir;
    strcat(core_path,core_file);

    snprintf(proc, sizeof(proc), "/proc/%s", (char*)strrchr(core_file,'-')+1);

    retry_count = 5;
    unsigned long last_size = 0;
    while(retry_count && !stat(proc, &st) && !stat(core_path,&st)) {
        if(last_size != (unsigned long)st.st_size) {
            last_size = st.st_size;
            retry_count = 5;
        } else {
            retry_count--;
        }
        MILOGD("%s size=%ldM",core_file,last_size/(1024*1024));
        sleep(1);
    }

    char* new_path = proc;

    sprintf(new_path, "%s/%s", logs_dir, core_file);
    MILOGD("rename %s to %s", core_path, new_path);
    rename(core_path,new_path);

    free(core_file);
}

static void do_compress(char* dir)
{
    char buf[PATH_MAX];
    snprintf(buf,sizeof(buf),"miuizip -o -9 -j %s.zip %s/*", dir, dir);
    system(buf);
}

#define SPEC_TAG "MiuiNativeDebug"
static void encript_zip(char* root_dir_name, char* basename)
{
    char filename[PATH_MAX];

    snprintf(filename, sizeof(filename), "%s.zip", basename);
#ifndef NDBG_LOCAL
    FILE *fp;
    if((fp = fopen(filename, "rb")) == NULL) {
        MILOGE("Can not open zip file.\n");
        return;
    }

    snprintf(filename, sizeof(filename), "%s.miz", basename);
    FILE *efp;
    if((efp = fopen(filename, "wb")) == NULL) {
        MILOGE("Fail to open to write z.\n");
        fclose(fp);
        return;
    }

    unsigned char iv_enc[AES_BLOCK_SIZE] = SPEC_TAG;
    for (int i = 0; i != AES_BLOCK_SIZE; ++i) {
        if (iv_enc[i] == 'i') {
            iv_enc[i] = 'u';
        } else if (iv_enc[i] == 'u') {
            iv_enc[i] = 'e';
        } else if (iv_enc[i] == 'e') {
            iv_enc[i] = 'o';
        }
    }
    iv_enc[AES_BLOCK_SIZE - 1] = 'X';

    unsigned char aes_key[AES_BLOCK_SIZE];
    int aes_key_index = 0;

    int bn_len = strlen(basename);
    for (int i = bn_len - 1; i && aes_key_index != AES_BLOCK_SIZE; --i) {
        if ('0' <= basename[i] && basename[i] <= '9') {
            aes_key[aes_key_index] = basename[i];
            ++ aes_key_index;
        }
    }
    for (int i = 0; iv_enc[i] != '\0' && aes_key_index != AES_BLOCK_SIZE; ++i) {
        aes_key[aes_key_index] = iv_enc[i];
        ++ aes_key_index;
    }

    fwrite(SPEC_TAG, AES_BLOCK_SIZE, 1, efp);

    AES_KEY enc_key;
    AES_set_encrypt_key(aes_key, AES_BLOCK_SIZE * 8, &enc_key);
    unsigned char switch_buf;
    unsigned char buffer[AES_BLOCK_SIZE];
    unsigned char encbuffer[AES_BLOCK_SIZE];
    size_t bytes_read = 0;
    while (!!(bytes_read = fread(buffer, 1, AES_BLOCK_SIZE, fp))) {
        AES_cbc_encrypt(buffer, encbuffer, bytes_read, &enc_key, iv_enc, AES_ENCRYPT);
        for (int i = 0; i != AES_BLOCK_SIZE / 2; ++ i) {
            switch_buf = encbuffer[i];
            encbuffer[i] = encbuffer[AES_BLOCK_SIZE - 1 - i];
            encbuffer[AES_BLOCK_SIZE - 1 - i] = switch_buf;
        }
        fwrite(encbuffer, AES_BLOCK_SIZE, 1, efp);
    }
    fclose(fp);
    fclose(efp);
#endif
    chown(filename, AID_SYSTEM, AID_SYSTEM);
    char buf[PATH_MAX];
#ifdef NDBG_LOCAL
    snprintf(buf, sizeof(buf), "mv %s %s/zip_local/", filename, root_dir_name);
#else
    snprintf(buf, sizeof(buf), "mv %s %s/zip/", filename, root_dir_name);
#endif
    system(buf);
}

static void clear_dir(char* dir)
{
    DIR *d;
    struct dirent *de;
    char path[PATH_MAX];

    strcpy(path,dir);
    d = opendir(dir);
    if(d) {
        while ((de = readdir(d)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")
#ifdef NDBG_LOCAL
                || !strcmp(de->d_name,"rules_local.xml")
                || (de->d_type == DT_DIR && !strcmp(de->d_name,"zip_local"))) {
#else
                || (de->d_type == DT_DIR && !strcmp(de->d_name,"zip"))) {
#endif
                continue;
            }
            snprintf(path,sizeof(path),"%s/%s", dir, de->d_name);
            if(de->d_type == DT_DIR) {
                clear_dir(path);
            }
            remove(path);
        }
        closedir(d);
    }
}

static bool check_zip_dir(char* root)
{
    char path[PATH_MAX];

#ifdef NDBG_LOCAL
    snprintf(path,sizeof(path),"%s/zip_local", root);
#else
    snprintf(path,sizeof(path),"%s/zip", root);
#endif

    if (access(path,F_OK)){
        if(mkdir(path, S_IRWXU | S_IXGRP | S_IXOTH)) {
            return true;
        }
        chown(path, AID_SYSTEM, AID_SYSTEM);
    }

    return false;
}

static bool try_lock(char* root)
{
    char path[PATH_MAX];

    snprintf(path,sizeof(path),"%s/lock", root);

    if(access(path, F_OK)) {
        int fd = creat(path, S_IRUSR | S_IWUSR);
        if (fd > 0) {
            close(fd);
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        return -1;
    }

    char* dir = argv[1];
    char root[PATH_MAX];

    strcpy(root,dir);
    *(char*)strrchr(root,'/') = '\0';

    if(check_zip_dir(root) || try_lock(root)) {
        return -1;
    }

    wait_core(dir);

    wait_other(dir);

    do_compress(dir);

    disable_coredump();

    //encript_zip(root,dir);

    clear_dir(root);

    // upload_zip();

    return 0;
}

