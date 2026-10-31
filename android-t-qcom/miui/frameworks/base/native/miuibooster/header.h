//
// req and resp packer header
// Created by dk on 2017/6/7.
//

#ifndef HARDCODER_HEADER_H
#define HARDCODER_HEADER_H

#include "util.h"

const static uint16_t HEADER_PROTOCAL_VERSION = 4;
const static uint32_t HEADER_BEGIN = 0x48444352; //HDCR

typedef struct AMCReqHeader {
    uint32_t begin;     // 包头其起始字段
    uint16_t version;   // 协议版本
    uint16_t funcid;    // 请求对应的function ID
    uint32_t bodylen;   // 包体序列化数据长度
    int64_t requestid;  // 当前请求包ID
    uint32_t callertid; // 上层JNI调用者所在线程ID
    int64_t timestamp;  // 当前请求时间戳

}__attribute__ ((packed)) AMCReqHeader;


static int64_t genReqPack(uint32_t funcid, uint8_t *data, int dataLen, uint8_t **outPack, uint32_t *outLen, uint32_t callertid, int64_t timestamp) {
    pdbg("getReqPack funcid:%d, dataLen:%d callertid:%d timestamp:%d", funcid, dataLen, callertid, TOINT(timestamp/1000000L));

    if (dataLen > 0 && data == NULL) {
        perr("getReqPack input data len %d data:%d", dataLen,  TOINT(data));
        return -1;
    }

    if (funcid > 65535) {
        perr("getReqPack funcid should little then 65535 %d ", funcid);
        return -2;
    }


    uint32_t totalLen = dataLen + sizeof(AMCReqHeader);
    uint8_t *buf = new uint8_t[totalLen];

    AMCReqHeader *header = (AMCReqHeader *) buf;
    header->begin =  HEADER_BEGIN;
    header->version = HEADER_PROTOCAL_VERSION;
    header->funcid = funcid;
    header->bodylen = dataLen;
    header->callertid = callertid;
    header->timestamp = timestamp;
    struct timeval now;
    gettimeofday(&now, NULL);
    header->requestid = (((int64_t) now.tv_sec) * 1000 * 1000) + now.tv_usec;
    if (dataLen) memcpy(buf + sizeof(AMCReqHeader), data, dataLen);

    *outPack = buf;
    *outLen = totalLen;

    return header->requestid;
}


typedef struct AMCRespHeader {
    uint32_t begin;     // 包头其起始字段
    uint16_t version;   // 协议版本
    uint16_t funcid;    // 响应请求对应的function ID
    uint32_t retCode;   // 请求处理结果
    uint32_t bodylen;   // 包体序列化数据长度
    int64_t requestid;  // 响应对应的请求包ID
    int64_t timestamp;  // 当前响应时间戳

}__attribute__ ((packed)) AMCRespHeader;


static int64_t genRespPack(uint32_t funcid, uint32_t retCode, uint64_t requestid, uint8_t *data, int dataLen,
                           uint8_t **outPack, uint32_t *outLen) {

    if (funcid > 65535) {
        perr("genRespPack funcid should little then 65535 %d ", funcid);
        return -2;
    }

    uint32_t totalLen = dataLen + sizeof(AMCRespHeader);
    uint8_t *buf = new uint8_t[totalLen];
    AMCRespHeader *p = (AMCRespHeader *) buf;
    p->retCode = retCode;
    p->requestid = requestid;
    p->funcid = funcid;
    p->bodylen = dataLen;
    struct timespec ts = {0, 0};
    clock_gettime(CLOCK_BOOTTIME_ALARM, &ts);
    p->timestamp = (((int64_t) ts.tv_sec) * 1000 * 1000 * 1000) + ts.tv_nsec;
    pdbg("genRespPack funcid:%d timestamp:%dms", funcid, TOINT(p->timestamp/1000000L));
    p->begin =  HEADER_BEGIN;
    p->version = HEADER_PROTOCAL_VERSION;

    if (dataLen) memcpy(buf + sizeof(AMCRespHeader), data, dataLen);
    *outPack = buf;
    *outLen = totalLen;
    return requestid;
}

#endif //HARDCODER_HEADER_H
