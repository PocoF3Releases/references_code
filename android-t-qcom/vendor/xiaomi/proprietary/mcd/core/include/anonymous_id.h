#ifndef OCTVM_ANONYMOUS_ID_H
#define OCTVM_ANONYMOUS_ID_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define SSL_PORT           443
#define SSL_HOST_NAME      "mid.sys.miui.com"
#define SSL_HOST_NAME_GLB  "mid.sys.intl.xiaomi.com"
#define SSL_REQUEST_PATH   "/api/v1/mid"

#define SERIALNO_PROP      "ro.serialno"
#define DEFAULT_IMEI_PROP  "ro.ril.miui.imei"
#define MID_FILE_PATH      "/data/system/mcd/mid"

/** return the mid in a plain string */
char *get_local_mid();

/** get the device attribute in a json string*/
char *get_device_attribute();

/** update the local mid from server*/
int update_local_mid(char *host, int port, /*char *CApath, char *CAfile,*/ int silent_mode);

/** encrypt the mid and write to file*/
void encrypt_mid(const char *ibuffer);

/** decrypt the mid from predefined file*/
int decrypt_mid(unsigned char *obuffer, size_t size);

static inline int raw_read_stdin(void *buf,int siz)
{
    return read(fileno(stdin),buf,siz);
}
static inline int raw_write_stdout(const void *buf,int siz)
{
    return write(fileno(stdout),buf,siz);
}

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif //OCTVM_ANONYMOUS_ID_H
