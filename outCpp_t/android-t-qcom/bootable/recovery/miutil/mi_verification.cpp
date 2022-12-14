/**
 * Copyright (c) XiaoMi Mobile.  All Rights Reserved.
 *
 * mi_verification.c
 *
 * Implementation of recovery permission check.
 *
 * @author Anson Zhang <zhangyang_b@xiaomi.com>
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>

#include "cutils/properties.h"

#include "recovery_utils/roots.h"
#include "miutil/mi_verification.h"
#include <setjmp.h>

extern "C" {
#include "miutil/mi_serial.h"
}

const char *base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char *public_key = (unsigned char *)"-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAs3X/eMU56OEc5buvfnXe\n\
D+9X4xKsImY7Q3Y+FY/xcdRIIvypKPjqxgqV548+Vf/H2VHeUXsMgiaeAqbmNAwP\n\
BU0He0p0YlUEOZ9Bkn6dKniuEe9G7PeL3tx6qjnf+Y5actf7IwJBTGJefa7wt+OZ\n\
4pNCBfgPHkTW+3VGlioP8rHbnLyZ98BdNxrvcLqJHCTblwomhVR64syqP1/eVOqE\n\
BKeHT+IkTXFZAzEVGha0FyKgBwcET/851zW93FiiC1HYpubLSiK1crbtDVK2ho4z\n\
NWzHCPfAfKQQn1VqXthiwxTZ25i4FYbuySwZ7xfXhqJwcLKuxN9uRCHBG8KV9V5T\n\
7wIDAQAB\n\
-----END PUBLIC KEY-----";

static int base64_decode(const char *base64, unsigned char *bindata)
{
    int i, j;
    unsigned char k;
    unsigned char temp[4];
    for (i = 0, j = 0; base64[i] != '\0'; i += 4) {
        memset(temp, 0xFF, sizeof(temp));
        for (k = 0; k < 64; k ++) {
            if (base64char[k] == base64[i])
                temp[0]= k;
        }
        for (k = 0; k < 64; k ++) {
            if (base64char[k] == base64[i+1])
                temp[1]= k;
        }
        for (k = 0; k < 64; k ++) {
            if (base64char[k] == base64[i+2])
                temp[2]= k;
        }
        for (k = 0; k < 64; k ++) {
            if (base64char[k] == base64[i+3])
                temp[3]= k;
        }

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
                ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
        if (base64[i+2] == '=')
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
                ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
        if (base64[i+3] == '=')
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
                ((unsigned char)(temp[3]&0x3F));
    }
    return j;
}

static int decrypt_identity(const char *msg, unsigned char **plain_str)
{
    printf("Decrypt identity start %s\n", msg);
    BIO *bio = BIO_new_mem_buf(public_key, -1);
    if (bio == NULL) {
        printf("Decrypt identity bio is NULL. \n");
        return -1;
    }
    RSA *rsa_public = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
    BIO_set_close(bio, BIO_CLOSE);
    BIO_free(bio);
    if (rsa_public == NULL) {
        printf("Decrypt identity rsa_public is NULL.\n");
        return -1;
    }
    int rsa_len = RSA_size(rsa_public);
    unsigned char *crypt_str = (unsigned char *)malloc(strlen(msg)*3/4 + 4);
    if (crypt_str == NULL) {
        RSA_free(rsa_public);
        printf("crypt_str malloc blk fail\n");
        return -1;
    }
    memset(crypt_str, 0, strlen(msg)*3/4 + 4);
    base64_decode(msg, crypt_str);
    *plain_str = (unsigned char *)malloc(rsa_len);
    if (*plain_str == NULL) {
        free(crypt_str);
        RSA_free(rsa_public);
        printf("plain_str malloc blk fail\n");
        return -1;
    }
    memset(*plain_str, 0, rsa_len);
    if (RSA_public_decrypt(PERM_LEN, crypt_str, *plain_str, rsa_public, RSA_PKCS1_PADDING) < 0) {
        free(crypt_str);
        free(*plain_str);
        RSA_free(rsa_public);
        printf("Decrypt identity RSA_public_decrypt failed. \n");
        return -1;
    }
    printf("Decrypt identity successed. \n");
    free(crypt_str);
    RSA_free(rsa_public);
    return rsa_len;
}

static int get_dev_sn(char *result)
{
    char product_device[PROPERTY_VALUE_MAX] = { 0 };
    property_get("ro.product.device", product_device, "ooops");
    if (!strncmp(product_device, "omega", 5) || !strncmp(product_device, "nikel", 5)) {
        if (get_longcheer_sn(result) < 0) {
            printf("Error: Get longcheer's serial number.\n");
            return -1;
        }
    } else {
        if (get_miphone_sn(result) < 0) {
            printf("Error: Get miphone's serial number.\n");
            return -1;
        }
    }
    return 0;
}

static int get_dev_version(char *result)
{
    printf("Get package's version.\n");
    property_get(VERSION_KEY, result, "unknown");
    if (strcmp(result, "unknown") == 0) {
        return -1;
    }
    return 0;
}

static int get_dev_rom_zone(char *result)
{
    printf("Get device rom zone.\n");
    property_get(ROM_ZONE_KEY, result, "unknown");
    if (strcmp(result, "unknown") == 0) {
        return -1;
    }
    return 0;
}

static int map_path(const std::string& path, MemMapping* pMap, bool needs_mount)
{
    memset(pMap, 0, sizeof(*pMap));
    if (setup_install_mounts() != 0) {
        return -1;
    } else if (!path.empty()) {
        if (needs_mount) {
            if (path[0] == '@') {
                if (ensure_path_mounted(path.substr(1).c_str()) != 0) {
                    return -2;
                }
            } else {
                if (ensure_path_mounted(path.c_str()) != 0) {
                    return -2;
                }
            }
        }
        if (!pMap->MapFile(path)) {
            LOG(ERROR) << "failed to map file";
            return -3;
        }
        return 0;
    }
    return -4;
}

static void transform(unsigned char *sha, char *result)
{
    int i;
    for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&(result[i*2]), "%02x", sha[i]);
    }
    result[2*SHA_DIGEST_LENGTH] = '\0';
}

[[maybe_unused]]static int get_package_sha_path(const std::string& path, char *result, bool needs_mount)
{
    printf("Get package's sha.\n");
    MemMapping map;

    if (map_path(path, &map, needs_mount) != 0) {
        LOG(ERROR) << "get_package_sha failed to map file";
        return -1;
    }

    unsigned char *sha = (unsigned char *)malloc(SHA_DIGEST_LENGTH);
    if (sha == NULL) {
        LOG(ERROR) << "get_package_sha failed to allocate" << SHA_DIGEST_LENGTH << " bytes";
        return -1;
    }
    SHA1((const unsigned char *)map.addr, map.length, sha);

    transform(sha, result);
    free(sha);
    return 0;
}

static int get_package_sha_map(char *result, MemMapping* pMap)
{
    printf("Get package's sha bay MemMapping.\n");
    unsigned char *sha = (unsigned char *)malloc(SHA_DIGEST_LENGTH);
    if (sha == NULL) {
        LOG(ERROR) << "get_package_sha_map failed to allocate" << SHA_DIGEST_LENGTH << " bytes";
        return -1;
    }
    SHA1(pMap->addr, pMap->length, sha);

    transform(sha, result);
    free(sha);
    return 0;
}

int check_identity(const char *msg,  MemMapping* pMap)
{
    unsigned char *plain_msg = NULL;
    int len = decrypt_identity(msg, &plain_msg);
    if (len < 0) {
        printf("Decrypt identity failed. \n");
        return -1;
    }
    printf("check_identity start\n");
    const char *format = "{\"SN\":\"%[^\"]\",\"Version\":\"%[^\"]\",\"SHA1\":\"%[^\"]\",\"Result\":%[^}]}%s";
    char msg_sn[PROPERTY_VALUE_MAX] = {0};
    char dev_sn[PROPERTY_VALUE_MAX] = {0};
    char msg_ver[PROPERTY_VALUE_MAX+1] = {0};
    char dev_ver[PROPERTY_VALUE_MAX+1] = {0};
    char msg_sha[2*SHA_DIGEST_LENGTH+1] = {0};
    char package_sha[2*SHA_DIGEST_LENGTH+1] = {0};
    char result[2] = {'1'};
    char verify[2] = {'1'};
    char msg_rom_zone[PROPERTY_VALUE_MAX] = {0};
    char dev_rom_zone[PROPERTY_VALUE_MAX] = {0};

    char msg_tail[100] = {0};
    sscanf((const char *)plain_msg, format, msg_sn, msg_ver, msg_sha, result, msg_tail);
    free(plain_msg);
    int allow = atoi(result);

    // parse msg_tail.
    // the message may be like {“SN”:"sn string", "Version":"current version", "SHA1":"sha1", "Result":0}{"Verify":1}{“Zone”:1}
    // the msg_tail is the string {"Verify":1}{“Zone”:1}, but {"Verify":1} or {“Zone”:1} may be either emtpy.
    // i.e. the msg_tail will be one of these format: 1.  {"Verify":1}{“Zone”:1}  2. {"Verify":1} 3. {“Zone”:1} 4.empty
    if (strncmp("{\"Verify\":", msg_tail, 10) == 0) {
        // if {“Zone”:1} empty, msg_rom_zone will be empty too.
        sscanf((const char *)msg_tail, "{\"Verify\":%[^}]}{\"Zone\":%[^}]}", verify, msg_rom_zone);
    } else if (strncmp("{\"Zone\":", msg_tail, 8) == 0) {
        // {“Zone”:1} only
        sscanf((const char *)msg_tail, "{\"Zone\":%[^}]}", msg_rom_zone);
    } else if (msg_tail[0] != 0) {
        // msg_tail is empty, and neither match {“Zone”:1} nor {"Verify":1}
        printf("Invalid verification message format\n");
        return -1;
    }

    int status_dev_rom_zone = get_dev_rom_zone(dev_rom_zone);
    // if msg rom zone valid but device rom zone invalid, verify fails.
    if ((status_dev_rom_zone < 0) && msg_rom_zone[0] != 0) {
        printf("Failed to verify message, device rom zone invalid.\n");
        return -1;
    // verify rom zone when wo got valid devivce rom zone
    } else if ((status_dev_rom_zone == 0) && (strcmp(dev_rom_zone, msg_rom_zone) != 0)) {
        printf("Failed to verify message, rom zone mismatch.\n");
        return -1;
    } else if (get_dev_sn(dev_sn) < 0 || get_dev_version(dev_ver) < 0 ||
        get_package_sha_map(package_sha, pMap) < 0) {
        printf("Failed to get serial number, get version or get package_sha.\n");
        return -1;
    }
    printf("msg:dev\nversion=>%s:%s\nSN=>%s:%s\nSHA=>%s:%s\n", msg_ver, dev_ver,msg_sn, dev_sn,msg_sha, package_sha);
    if (strcmp(msg_ver, dev_ver) != 0 || strcmp(msg_sn, dev_sn) != 0 || strcmp(msg_sha, package_sha) != 0) {
        printf("Failed to verify message.\n");
        return -1;
    }
    return allow;
}

void sig_bus(int sig) {
    longjmp(jb, 1);
}

int check_verify(const char *msg, MemMapping* pMap){
    if (msg == NULL) {
        return 1;
    }

    unsigned char *plain_msg = NULL;
    int len = decrypt_identity(msg, &plain_msg);
    if (len < 0) {
        printf("Decrypt identity failed. \n");
        return -1;
    }
    printf("check_verify start\n");
    const char *format = "{\"SN\":\"%[^\"]\",\"Version\":\"%[^\"]\",\"SHA1\":\"%[^\"]\",\"Result\":%[^}]}%s";
    char msg_sn[PROPERTY_VALUE_MAX] = {0};
    char dev_sn[PROPERTY_VALUE_MAX] = {0};
    char msg_ver[PROPERTY_VALUE_MAX+1] = {0};
    char dev_ver[PROPERTY_VALUE_MAX+1] = {0};
    char msg_sha[2*SHA_DIGEST_LENGTH+1] = {0};
    char package_sha[2*SHA_DIGEST_LENGTH+1] = {0};
    char result[2] = {'1'};
    char verify[2] = {'1'};
    char msg_rom_zone[PROPERTY_VALUE_MAX] = {0};
    char dev_rom_zone[PROPERTY_VALUE_MAX] = {0};

    char msg_tail[100] = {0};
    sscanf((const char *)plain_msg, format, msg_sn, msg_ver, msg_sha, result, msg_tail);
    free(plain_msg);

    if (get_dev_sn(dev_sn) < 0 || get_dev_version(dev_ver) < 0) {
        printf("Failed to get serial number or get version\n");
        return -1;
    }
    printf("msg:dev\ncheck version=>%s:%s\nSN=>%s:%s\nverify=>%s\n", msg_ver, dev_ver,msg_sn, dev_sn,verify);
    if (strcmp(msg_sn, dev_sn) != 0 || strcmp(msg_ver, dev_ver) != 0) {
        printf("Failed to verify message.\n");
        return -1;
    }

    // parse msg_tail.
    // the message may be like {“SN”:"sn string", "Version":"current version", "SHA1":"sha1", "Result":0}{"Verify":1}{“Zone”:1}
    // the msg_tail is the string {"Verify":1}{“Zone”:1}, but {"Verify":1} or {“Zone”:1} may be either emtpy.
    // i.e. the msg_tail will be one of these format: 1.  {"Verify":1}{“Zone”:1}  2. {"Verify":1} 3. {“Zone”:1} 4.empty
    if (strncmp("{\"Verify\":", msg_tail, 10) == 0) {
        // if {“Zone”:1} empty, msg_rom_zone will be empty too.
        sscanf((const char *)msg_tail, "{\"Verify\":%[^}]}{\"Zone\":%[^}]}", verify, msg_rom_zone);
    } else if (strncmp("{\"Zone\":", msg_tail, 8) == 0) {
        // {“Zone”:1} only
        sscanf((const char *)msg_tail, "{\"Zone\":%[^}]}", msg_rom_zone);
    } else if (msg_tail[0] != 0) {
        // msg_tail is not empty, but neither match {“Zone”:1} nor {"Verify":1}
        printf("Invalid verification message format\n");
        return -1;
    }

    int check = atoi(verify);

    if (check) {
        int status_dev_rom_zone = get_dev_rom_zone(dev_rom_zone);
        // if msg rom zone valid but device rom zone invalid, verify fails.
        if ((status_dev_rom_zone < 0) && msg_rom_zone[0] != 0) {
            printf("Failed to verify message, device rom zone invalid.\n");
            return -1;
        // verify rom zone when wo got valid devivce rom zone
        } else if ((status_dev_rom_zone == 0) && (strcmp(dev_rom_zone, msg_rom_zone) != 0)) {
            printf("Failed to verify message, rom zone mismatch.\n");
            return -1;
        } else if (setjmp(jb) == 0) {
            if (get_package_sha_map(package_sha, pMap) < 0){
                printf("Failed to get package_sha.\n");
                return -1;
            }
        } else {
            LOG(ERROR) << "Compute package sha get SIGBUS when the host aborts the transfer.";
            return -1;
        }
        printf("check SHA=>%s:%s\n", msg_sha, package_sha);
        if (strcmp(msg_sha, package_sha) != 0) {
            printf("Failed to verify message.\n");
            return -1;
        }
    }

    return check;
}
