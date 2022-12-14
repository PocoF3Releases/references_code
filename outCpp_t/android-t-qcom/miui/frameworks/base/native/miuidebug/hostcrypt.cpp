/*
 * Encrypt and decrypt tool for miui native debug file.
 *
 * USAGE: mnd_crypt [OPTIONS] [FILE]
 *
 * OPTION:
 * -e,  encrypt file
 * -d,  decrypt file
 * -v version-number,  specific version, it is just needed for rules.xml file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/aes.h>

#define KEY_LEN 128
#define STR_MAX_LEN 256
#define SPECIAL_TAG "MiuiNativeDebug"
#define EXTENSION "miz"

#define PRINT_HERE printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);


unsigned char s_key[AES_BLOCK_SIZE];
unsigned char s_iv[AES_BLOCK_SIZE] = SPECIAL_TAG;

static void hex_print(const void* pv, size_t len) {
    const unsigned char * p = (const unsigned char*)pv;
    if (NULL == pv) {
        printf("NULL");
    } else {
        for (size_t i = 0; i < len ; ++i) {
            printf("%02X ", *p++);
        }
    }
    printf("\n");
}

void init_iv() {
    for (int i = 0; i != AES_BLOCK_SIZE; ++i) {
        if (s_iv[i] == 'i') {
            s_iv[i] = 'u';
        } else if (s_iv[i] == 'u') {
            s_iv[i] = 'e';
        } else if (s_iv[i] == 'e') {
            s_iv[i] = 'o';
        }
    }
    s_iv[AES_BLOCK_SIZE - 1] = 'X';
}

void fillup_key(int s_key_index) {
    for (int i = 0; s_key_index != AES_BLOCK_SIZE; ++i) {
        s_key[s_key_index] = s_iv[i];
        ++ s_key_index;
    }
}

void rule_crypt_init(char *version) {
    init_iv();
    int s_key_index = 0;
    for (int i = 0; version[i] != '\0' && s_key_index != AES_BLOCK_SIZE; ++i) {
        if ('0' <= version[i] && version[i] <= '9') {
            s_key[s_key_index] = version[i];
            ++s_key_index;
        }
    }
    fillup_key(s_key_index);
}

void miz_crypt_init(char *filename) {
    init_iv();
    int s_key_index = 0;

    // e.g. 16041401-20160607160919-6.6.6alpha-aries-5135fd22.miz_1465287029000
    char *p = strstr(strstr(strstr(filename, "_") + 2, "_") + 2, "_");
    if (!p) {
        // e.g. 16041401-20160607160919-6.6.6.miz
        p = strstr(filename, EXTENSION);
    }
    for (; p != filename && s_key_index != AES_BLOCK_SIZE; --p) {
        if ('0' <= *p && *p <= '9') {
            s_key[s_key_index] = *p;
            ++s_key_index;
        }
    }
    fillup_key(s_key_index);
}

void encrypt_miui_native_file(char *filename) {
    FILE *fp;
    if ((fp = fopen(filename,"rb")) == NULL) {
        printf("Can not open to read file.\n");
        return;
    }

    FILE *efp;
    char encfilename[STR_MAX_LEN];
    snprintf(encfilename, STR_MAX_LEN, "%s.enc", filename);
    if ((efp = fopen(encfilename,"wb")) == NULL) {
        printf("Can not open to write file.\n");
        fclose(fp);
        return;
    }

    AES_KEY enc_key;
    AES_set_encrypt_key(s_key, AES_BLOCK_SIZE * 8, &enc_key);

    unsigned char switch_buf;
    unsigned char buffer[AES_BLOCK_SIZE];
    unsigned char encbuffer[AES_BLOCK_SIZE];
    fwrite(SPECIAL_TAG, AES_BLOCK_SIZE, 1, efp);

    size_t bytes_read = 0;
    while (!!(bytes_read = fread(buffer, 1, AES_BLOCK_SIZE, fp))) {
        AES_cbc_encrypt(buffer, encbuffer, bytes_read, &enc_key, s_iv, AES_ENCRYPT);
        for (int i = 0; i != AES_BLOCK_SIZE / 2; ++ i) {
            switch_buf = encbuffer[i];
            encbuffer[i] = encbuffer[AES_BLOCK_SIZE - 1 - i];
            encbuffer[AES_BLOCK_SIZE - 1 - i] = switch_buf;
        }
        fwrite(encbuffer, AES_BLOCK_SIZE, 1, efp);
    }

    fclose(fp);
    fclose(efp);
    printf("Encript successfully! >> %s\n", encfilename);
}

void decrypt_miui_native_file(char *filename) {
    FILE *fp;
    if ((fp = fopen(filename,"rb")) == NULL) {
        printf("Can not open to read file.\n");
        return;
    }

    FILE *dfp;
    char decfilename[STR_MAX_LEN];
    snprintf(decfilename, STR_MAX_LEN, "%s.out", filename);
    if ((dfp = fopen(decfilename,"wb")) == NULL) {
        printf("Can not open to write file.\n");
        fclose(fp);
        return;
    }

    AES_KEY dec_key;
    AES_set_decrypt_key(s_key, AES_BLOCK_SIZE * 8, &dec_key);

    unsigned char switch_buf;
    unsigned char buffer[AES_BLOCK_SIZE];
    unsigned char decbuffer[AES_BLOCK_SIZE];
    fread(buffer, AES_BLOCK_SIZE, 1, fp);

    if (memcmp(buffer, SPECIAL_TAG, AES_BLOCK_SIZE)) {
        printf("Format is wrong!\n");
        fclose(fp);
        fclose(dfp);
        return;
    }

    while (fread(buffer, 1, AES_BLOCK_SIZE, fp)) {
        for (int i = 0; i != AES_BLOCK_SIZE / 2; ++ i) {
            switch_buf = buffer[i];
            buffer[i] = buffer[AES_BLOCK_SIZE - 1 - i];
            buffer[AES_BLOCK_SIZE - 1 - i] = switch_buf;
        }
        AES_cbc_encrypt(buffer, decbuffer, AES_BLOCK_SIZE, &dec_key, s_iv, AES_DECRYPT);
        fwrite(decbuffer, AES_BLOCK_SIZE, 1, dfp);
    }

    fclose(fp);
    fclose(dfp);
    printf("Decript successfully! >> %s\n", decfilename);
}

int main(int argc, char* const argv[]) {
    bool encrypt = false;
    char *filename = NULL;
    char *version = NULL;

    for (int i = 1; i != argc; ++i) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'e') {
                encrypt = true;
            } else if (argv[i][1] == 'd') {
                encrypt = false;
            } else if (argv[i][1] == 'v') {
                ++i;
                version = argv[i];
            } else {
                printf("Unrecognize argument, %s", argv[i]);
            }
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        printf("Filename needed!\n");
        exit(-1);
    }

    if (strstr(filename, "rules")) {
        if (!version) {
            printf("Rule file need build version!\n");
            exit(-1);
        }
        rule_crypt_init(version);
    } else {
        miz_crypt_init(filename);
    }

    if (encrypt) {
        encrypt_miui_native_file(filename);
    } else {
        decrypt_miui_native_file(filename);
    }
    return 0;
}
