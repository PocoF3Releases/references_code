#ifndef _LIB_API_H
#define _LIB_API_H

#ifdef __cplusplus
extern "C" {
#endif
#define IN
#define OUT
#define MAX_RTCM_LEN 20480
#define MAX_MSG_LEN 20480
#define MAX_ENCRY_MSG_LEN 40960
#define MAX_GGA_LEN 2560
#define MAX_CACHE_LEN 20480

typedef char uint8;
typedef void (*SdkRtcmData)(void *data, int length, long count);
typedef void (*SdkRtcmStatus)(int status);

typedef enum ENUM_COOR {
    CGCS2000 = 8001,
    WGS84 = 8002,
    ITRF2000 = 8003
} ENUM_COOR;

typedef struct _rtcm_data_response_{
    SdkRtcmData cb_rtcmdata;
} RtcmDataResponse;

typedef struct _rtcm_status_response_{
    SdkRtcmStatus cb_status;
} RtcmStatusResponse;

/* This is a non-blocking API for send GGA to CORS
 *
 * @param[in]  ggaRawBuf : data to send in ASCII character
 * @param[in]  length    : data len to send in ASCII character
 *
 * @return:
 *   0 if success
 *  -1 if fail
 */
int sendGGA(IN  const char* ggaRawBuf, IN const int length);

void handle_status_rsp(int errType);

/**
 * @brief : get SDK version
 *
 * @return:
 *   a pointer to version
 */
const char* getSdkVersion(void);

int getLength(void);
char* getAnsBuf(void);
unsigned long getRecvCnt(void);

/**
 * @brief : start SDK statemachine
 * @param[in]  data_rsp  : include a callback function for pass the rtcm data
 * @param[in]  status_rsp: include a callback function for pass the rtcm status
 * @return:
 *   void
 */
void startSdk(RtcmDataResponse *data_rsp, RtcmStatusResponse *status_rsp);

/**
 * @brief : stop SDK statemachine
 * @return:
 *   void
 */
void stopSdk();

/* This is a Synchronous block API for get RTCM data from CORS platform(only for DEMO)
 *
 * @param[in]  data: best pos data
 * @param[in]  size: data size
 *
 * @return:
 *   0 if success
 *  -1 if fail
 */
int sendBestPos(IN  const void* data,
                IN  int         size);

/**
 * @brief : 
 * @return:
 *   void
 */
int
setUserPasswd(IN char* server_ip,
              IN char* user,
              IN char* pwd,
              IN char* source_id,
              IN int   port);

uint8 setRtcmUserInfo(char *name, char *password);

int setDeviceID(IN char* device_id);

char * getDeviceID();

int setCoor(ENUM_COOR coor);
int getCoor();
void getExpireDate(char* expire_day, int length);
void init();
void clear();
#ifdef __cplusplus
}
#endif

#endif//_LIB_API_H

