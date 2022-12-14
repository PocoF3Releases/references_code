//
// protocal const definition and implement
// Created by dk on 17/4/18.
//

#ifndef AMC_PROTOCOL_H
#define AMC_PROTOCOL_H

#include "protos/amc.pb.h"
#include "header.h"

// HC register call back type
const static uint32_t REGISTER_CALLBACK_TYPE_SYSTEM_EVENT = 0x1;
const static uint32_t REGISTER_CALLBACK_TYPE_ANR = 0x2;
// HC response data type
const static uint32_t RESPONSE_DATA_TYPE_GETPARAMETER = 0x10;

// HC system event definition
const static uint32_t SYSTEM_EVENT_BASE = 0x0;
const static uint32_t SYSTEM_EVENT_SLIDE_OPEN = SYSTEM_EVENT_BASE + 1;
const static uint32_t SYSTEM_EVENT_SLIDE_CLOSE = SYSTEM_EVENT_BASE + 2;

const string SYSTEM_EVENT_CODE = "system_event_code";

const static int32_t RET_OK = 0;

//requestCpuHighFreq，requestHighIOFreq 直接返回level n
const static int32_t RET_LEVEL_1 = 1;
const static int32_t RET_LEVEL_2 = 2;
const static int32_t RET_LEVEL_3 = 3;

//预留返回值最后三位作为level，倒数第四位代表cpu level，倒数第五位代表io level，新增值继续左移
const static int32_t RET_CPU_HIGH_FREQ = 1 << 3;// 1000，即8
const static int32_t RET_HIGH_IO_FREQ = 1 << 4; // 10000，即16


//requestUnifyCpuIOThreadCore使用复合标识位
const static int32_t RET_CPU_HIGH_FREQ_LEVEL_1 = RET_CPU_HIGH_FREQ | RET_LEVEL_1;   //Unify接口返回cpu level 1，1000 | 01 = 1001
const static int32_t RET_CPU_HIGH_FREQ_LEVEL_2 = RET_CPU_HIGH_FREQ | RET_LEVEL_2;   //Unify接口返回cpu level 2，1000 | 10 = 1010
const static int32_t RET_CPU_HIGH_FREQ_LEVEL_3 = RET_CPU_HIGH_FREQ | RET_LEVEL_3;   //Unify接口返回cpu level 3，1000 | 11 = 1011

const static int32_t RET_HIGH_IO_FREQ_LEVEL_1 = RET_HIGH_IO_FREQ | RET_LEVEL_1;     //Unify接口返回io level 1，10000 | 01 = 10001
const static int32_t RET_HIGH_IO_FREQ_LEVEL_2 = RET_HIGH_IO_FREQ | RET_LEVEL_2;     //Unify接口返回io level 2，10000 | 10 = 10010
const static int32_t RET_HIGH_IO_FREQ_LEVEL_3 = RET_HIGH_IO_FREQ | RET_LEVEL_3;     //Unify接口返回io level 3，10000 | 11 = 10011


const static int32_t ERR_UNAUTHORIZED = -10001;
const static int32_t ERR_FUNCTION_NOT_SUPPORT = -10002;
const static int32_t ERR_SERVICE_UNAVAILABLE = -10003;
const static int32_t ERR_FAILED_DEPENDENCY = -10004;
const static int32_t ERR_PACKAGE_DECODE_FAILED = -10005;
const static int32_t ERR_PARAMETERS_WRONG = -10006;
const static int32_t ERR_CLIENT_UPGRADE_REQUIRED = -10007;

const static int32_t ERR_CLIENT_DISCONNECT = -20001;
const static int32_t ERR_CLIENT_RESPONSE = -20002;


// FUNC ID BEGIN  ,  2 ^ 16 IS MAX
const static uint32_t FUNC_BASE = 1000;

const static uint32_t FUNC_CHECK_PERMISSION = FUNC_BASE + 1;

const static uint32_t FUNC_CPU_HIGH_FREQ = FUNC_BASE + 2;
const static uint32_t FUNC_CANCEL_CPU_HIGH_FREQ = FUNC_BASE  + 3;

const static uint32_t FUNC_CPU_CORE_FOR_THREAD = FUNC_BASE + 4;
const static uint32_t FUNC_CANCEL_CPU_CORE_FOR_THREAD = FUNC_BASE + 5;

const static uint32_t FUNC_HIGH_IO_FREQ = FUNC_BASE + 6;
const static uint32_t FUNC_CANCEL_HIGH_IO_FREQ = FUNC_BASE + 7;

const static uint32_t FUNC_SET_SCREEN_RESOLUTION = FUNC_BASE + 8;
const static uint32_t FUNC_RESET_SCREEN_RESOLUTION = FUNC_BASE + 9;

const static uint32_t FUNC_REG_ANR_CALLBACK = FUNC_BASE + 10;

const static uint32_t FUNC_REG_PRELOAD_BOOT_RESOURCE = FUNC_BASE + 11;

const static uint32_t FUNC_TERMINATE_APP = FUNC_BASE + 12;

const static uint32_t FUNC_UNIFY_CPU_IO_THREAD_CORE_GPU = FUNC_BASE + 13;
const static uint32_t FUNC_CANCEL_UNIFY_CPU_IO_THREAD_CORE_GPU = FUNC_BASE + 14;

const static uint32_t FUNC_REG_SYSTEM_EVENT_CALLBACK = FUNC_BASE + 15;

const static uint32_t FUNC_GPU_HIGH_FREQ = FUNC_BASE + 16;
const static uint32_t FUNC_CANCEL_GPU_HIGH_FREQ = FUNC_BASE + 17;

const static uint32_t FUNC_CONFIGURE = FUNC_BASE + 18;

const static uint32_t FUNC_GET_PARAMETERS = FUNC_BASE + 19;

const static uint32_t GETPARAMETERS_TYPE_BASE = 0;
const static uint32_t GETPARAMETERS_TYPE_QP_MEDIACODEC = GETPARAMETERS_TYPE_BASE + 1;

const static uint32_t STATUS_NONE = 0;
const static uint32_t STATUS_REQUEST = 1;
const static uint32_t STATUS_RESET = 2;

// FUNC ID END  ,  2 ^ 16 IS MAX

// LEVEL
const static uint32_t CPU_LEVEL_1 = 1; // highest
const static uint32_t CPU_LEVEL_2 = 2;
const static uint32_t CPU_LEVEL_3 = 3;


const static uint32_t IO_LEVEL_1 = 1; // highest
const static uint32_t IO_LEVEL_2 = 2;
const static uint32_t IO_LEVEL_3 = 3;

// SCENEN
const static uint32_t SCENE_BOOT = 101;

const static uint32_t SCENE_RECEVE_MSG = 201;
const static uint32_t SCENE_SEND_MSG = 202;
const static uint32_t SCENE_SEND_PIC_MSG = 203;

const static uint32_t SCENE_ENTER_CHATTING = 301;
const static uint32_t SCENE_QUIT_CHATTING = 302;

const static uint32_t SCENE_UPDATE_CHATROOM = 401;

const static uint32_t SCENE_SCENE_DB = 501;

const static uint32_t SCENE_DECODE_PIC = 601;
const static uint32_t SCENE_GIF = 602;
const static uint32_t SCENE_ENCODE_VIDEO = 603;

const static uint32_t SCENE_SNS_SCROLL = 701;
const static uint32_t SCENE_ALBUM_SCROLL = 702;
const static uint32_t SCENE_MEDIA_GALLERY_SCROLL = 703;

// ACTION
const static int64_t ACTION_DEXO2OAT        = 1 << 0;   //CPUFreq, IOFreq
const static int64_t ACTION_ANIMATION       = 1 << 1;   //CPUFreq
const static int64_t ACTION_ONCREATE        = 1 << 2;   //CPUFreq
const static int64_t ACTION_ONDESTROY       = 1 << 3;   //CPUFreq
const static int64_t ACTION_INIT_LISTVIEW   = 1 << 4;   //CPUFreq
const static int64_t ACTION_SCROLL_LISTVIEW = 1 << 5;   //CPUFreq
const static int64_t ACTION_DECODE_PIC      = 1 << 6;   //CPUFreq, IOFreq, BindCore
const static int64_t ACTION_ENCODE_PIC      = 1 << 7;   //CPUFreq, IOFreq, BindCore
const static int64_t ACTION_DECODE_VIDEO    = 1 << 8;   //CPUFreq, IOFreq, BindCore
const static int64_t ACTION_ENCODE_VIDEO    = 1 << 9;   //CPUFreq, IOFreq, BindCore
const static int64_t ACTION_DECODE_STREAM   = 1 << 10;  //CPUFreq, BindCore voip
const static int64_t ACTION_ENCODE_STREAM   = 1 << 11;  //CPUFreq, BindCore voip
const static int64_t ACTION_QUERY_SQL       = 1 << 12;  //CPUFreq, IOFreq
const static int64_t ACTION_WRITE_SQL       = 1 << 13;  //CPUFreq, IOFreq
const static int64_t ACTION_READ_FILE       = 1 << 14;  //CPUFreq, IOFreq
const static int64_t ACTION_WRITE_FILE      = 1 << 15;  //CPUFreq, IOFreq
const static int64_t ACTION_ALLOC_MEMORY    = 1 << 16;  //CPUFreq
const static int64_t ACTION_NET_RX          = 1 << 17;
const static int64_t ACTION_NET_TX          = 1 << 18;

//HardCoder interface
class HardCoder {
public:

    virtual ~HardCoder(){}

    virtual int getUidByAddress(const char *ip, int port) {
        UNUSED(ip);
        UNUSED(port);
        return 0;
    }

    virtual bool checkPermission(int funcid, int uid, int callertid, int64_t timestamp) {
        UNUSED(funcid);
        UNUSED(uid);
        UNUSED(callertid);
        UNUSED(timestamp);
        return true;
    }

    virtual int requestCpuHighFreq(int scene, int64_t action, int level, int timeoutms, int callertid, int64_t timestamp) {
        UNUSED(scene);
        UNUSED(action);
        UNUSED(level);
        UNUSED(timeoutms);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int cancelCpuHighFreq(int callertid, int64_t timestamp) {
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int requestGpuHighFreq(int scene, int64_t action, int level, int timeoutms, int callertid, int64_t timestamp) {
        UNUSED(scene);
        UNUSED(action);
        UNUSED(level);
        UNUSED(timeoutms);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int cancelGpuHighFreq(int callertid, int64_t timestamp) {
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int requestCpuCoreForThread(int scene, int64_t action, vector<int>bindtids, int timeoutms, int callertid, int64_t timestamp) {
        UNUSED(scene);
        UNUSED(action);
        UNUSED(bindtids);
        UNUSED(timeoutms);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int cancelCpuCoreForThread(vector<int>bindtids, int callertid, int64_t timestamp) {
        UNUSED(bindtids);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int requestHighIOFreq(int scene, int64_t action, int level, int timeoutms, int callertid, int64_t timestamp) {
        UNUSED(scene);
        UNUSED(action);
        UNUSED(level);
        UNUSED(timeoutms);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int cancelHighIOFreq(int callertid, int64_t timestamp) {
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int requestScreenResolution(int level, string uiName, int callertid, int64_t timestamp) {
        UNUSED(level);
        UNUSED(uiName);
        UNUSED(level);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int resetScreenResolution(int callertid, int64_t timestamp) {
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int registerSystemEventCallback(int uid, int callertid, int64_t timestamp) {
        UNUSED(uid);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int registerAnrCallback(int uid, int callertid, int64_t timestamp) {
        UNUSED(uid);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int registerBootPreloadResource(vector<string> file, int callertid, int64_t timestamp) {
        UNUSED(file);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int terminateApp(int callertid, int64_t timestamp) {
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int requestUnifyCpuIOThreadCoreGpu(int scene, int64_t action, int cpulevel, int iolevel, vector<int>bindtids, int gpulevel, int timeoutms, int callertid, int64_t timestamp) {
        UNUSED(scene);
        UNUSED(action);
        UNUSED(cpulevel);
        UNUSED(iolevel);
        UNUSED(bindtids);
        UNUSED(gpulevel);
        UNUSED(timeoutms);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int cancelUnifyCpuIOThreadCoreGpu(int cancelcpu, int cancelio, int cancelthread, vector<int>bindtids, int cancelgpu, int callertid, int64_t timestamp) {
        UNUSED(cancelcpu);
        UNUSED(cancelio);
        UNUSED(cancelthread);
        UNUSED(bindtids);
        UNUSED(cancelgpu);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int configure(const string& data, int callertid, int64_t timestamp) {
        UNUSED(data);
        UNUSED(callertid);
        UNUSED(timestamp);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual int getParameters(const string& data, int callertid, int64_t timestamp, string& respData) {
        UNUSED(data);
        UNUSED(callertid);
        UNUSED(timestamp);
        UNUSED(respData);
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    virtual void onEvent(int eventCode) {
        UNUSED(eventCode);
    }

};

class IHardCoderDataCallback{
public:
    virtual int onCallback(int uid, int funcid, uint64_t requestid, uint64_t timestamp, int retCode,
                           uint8_t *data, int len) = 0;

    virtual ~IHardCoderDataCallback(){}
};

template<typename T>
class ServerDataWorker {

private:

    T remote;
    uint8_t *resp;
    uint32_t respLen;

public:
    ServerDataWorker() {
        resp = NULL;
        respLen = 0;
    }

    ~ServerDataWorker() {
        if (resp)delete[]resp;
        resp = NULL;
    }

    uint8_t *getResp() {
        return resp;
    }

    uint32_t getResLen() {
        return respLen;
    }

    T getRemote() {
        return remote;
    }

    int processSend(uint32_t funcid, T *r, uint8_t *data, uint32_t len) {
        remote = *r;
        int64_t ret = genRespPack(funcid, RET_OK, 0, data, len,  &resp, &respLen);
        pdbg("processSend 3 funcid:%d, len:%d, remote:%d ret:%d, respLen:%d", funcid, len, remote, TOINT(ret), respLen);
        return ret;
    }

    int processReceive(int uid, T *r,  AMCReqHeader *pHead,  uint8_t *payload, uint32_t payloadLen, HardCoder *inst) {

        if (r == NULL || (payload == NULL && payloadLen != 0)) {
            perr("recvEvent DATA data or addr invalid ret len:%d, %d", payloadLen, TOINT(sizeof(AMCReqHeader)));
            return -1;
        }

        remote = *r;


        if (pHead->version < HEADER_PROTOCAL_VERSION) {
            perr("CheckVersion failed:%d  server:%d", pHead->version, HEADER_PROTOCAL_VERSION);
            genRespPack(pHead->funcid, ERR_CLIENT_UPGRADE_REQUIRED, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (!inst->checkPermission(pHead->funcid, uid, pHead->callertid, pHead->timestamp)) {
            perr("checkPermission[%d, %d] failed ret ERR_UNAUTHORIZED", pHead->funcid, uid);
            genRespPack(pHead->funcid, ERR_UNAUTHORIZED, 0, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CHECK_PERMISSION) {
            int respResult = inst->checkPermission(pHead->funcid, uid, pHead->callertid, pHead->timestamp) ? 0 : ERR_UNAUTHORIZED;
            pdbg("FUNC_CHECK_PERMISSION [%d,%d] resp:%d", pHead->funcid, uid, respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_UNIFY_CPU_IO_THREAD_CORE_GPU) {
            amc::RequestUnifyCPUIOThreadCore p;
            int ret = p.ParseFromArray(payload, payloadLen);
            vector<int> vec;
            if (ret) {
                int size = p.bindtids_size();
                for (int i = 0; i < size; i++) {
                    vec.push_back(p.bindtids(i));
                }
            }
            int respResult = ret ? inst->requestUnifyCpuIOThreadCoreGpu(p.scene(),p.action(), p.cpulevel(), p.iolevel(), vec, p.gpulevel(), p.timeoutms(), pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_UNIFY_CPU_IO_THREAD_CORE parse:%d [%d,%d,%d,%d,%d,%d,%d] resp:%d", ret, p.scene(), TOINT(p.action()), p.cpulevel(), p.iolevel(), TOINT(p.bindtids_size()), p.gpulevel(), p.timeoutms(), respResult);
            /***
            ** refer to header.h, the second parameter of genRespPack is retCode,
            ** refer to function "onCallBack (this is client codes)", retCode means error code.
            ** so set errorcode to 0, if this is no error.
            ** and the fourth and fifth parameter of genRespPack, it's the callback data response to client.
            ** in this case, callback data is not used, so set to NULL.
            ***/
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CANCEL_UNIFY_CPU_IO_THREAD_CORE_GPU) {
            amc::CancelUnifyCPUIOThreadCore p;
            int ret = p.ParseFromArray(payload, payloadLen);
            vector<int> vec;
            if (ret) {
                int size = p.bindtids_size();
                for (int i = 0; i < size; i++) {
                    vec.push_back(p.bindtids(i));
                }
            }
            int respResult = ret ? inst->cancelUnifyCpuIOThreadCoreGpu(p.cancelcpu(), p.cancelio(), p.cancelthread(), vec,p.cancelgpu(),  pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_CANCEL_UNIFY_CPU_IO_THREAD_CORE parse:%d [%d,%d,%d,%d,%d] resp:%d", ret, p.cancelcpu(), p.cancelio(), p.cancelthread(), TOINT(p.bindtids_size()), p.cancelgpu(), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CPU_HIGH_FREQ) {
            amc::RequestCPUHighFreq p;
            int ret = p.ParseFromArray(payload, payloadLen);
            int respResult = ret ? inst->requestCpuHighFreq(p.scene(),p.action(), p.level(), p.timeoutms(), pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_CPU_HIGH_FREQ parse:%d [%d,%d,%d,%d] resp:%d", ret, p.scene(), TOINT(p.action()), p.level(), TOINT(p.timeoutms()), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CANCEL_CPU_HIGH_FREQ) {
            int respResult = inst->cancelCpuHighFreq(pHead->callertid, pHead->timestamp);
            pdbg("FUNC_CANCEL_CPU_HIGH_FREQ resp:%d", respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_GPU_HIGH_FREQ) {
            amc::RequestGPUHighFreq p;
            int ret = p.ParseFromArray(payload, payloadLen);
            int respResult = ret ? inst->requestGpuHighFreq(p.scene(),p.action(), p.level(), p.timeoutms(), pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_GPU_HIGH_FREQ parse:%d [%d,%d,%d,%d] resp:%d", ret, p.scene(), TOINT(p.action()), p.level(), TOINT(p.timeoutms()), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CANCEL_GPU_HIGH_FREQ) {
            int respResult = inst->cancelGpuHighFreq(pHead->callertid, pHead->timestamp);
            pdbg("FUNC_CANCEL_GPU_HIGH_FREQ resp:%d", respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CPU_CORE_FOR_THREAD) {
            amc::RequestCPUCoreForThread p;
            int ret = p.ParseFromArray(payload, payloadLen);
            vector<int> vec;
            if (ret) {
                int size = p.bindtids_size();
                for (int i = 0; i < size; i++) {
                    vec.push_back(p.bindtids(i));
                }
            }
            int respResult = ret ? inst->requestCpuCoreForThread(p.scene(), p.action(), vec, p.timeoutms(), pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_CPU_CORE_FOR_THREAD  parse:%d [%d,%d,%d,%d] resp:%d", ret, p.scene(),TOINT(p.action()), p.bindtids_size(), TOINT(p.timeoutms()), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CANCEL_CPU_CORE_FOR_THREAD) {
            amc::CancelCPUCoreForThread p;
            int ret = p.ParseFromArray(payload, payloadLen);
            vector<int> vec;
            if (ret) {
                int size = p.bindtids_size();
                for (int i = 0; i < size; i++) {
                    vec.push_back(p.bindtids(i));
                }
            }
            int respResult = ret ? inst->cancelCpuCoreForThread(vec, pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_CANCEL_CPU_CORE_FOR_THREAD  parse:%d [%d] resp:%d", ret, p.bindtids_size(), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_HIGH_IO_FREQ) {
            amc::RequestHighIOFreq p;
            int ret = p.ParseFromArray(payload, payloadLen);
            int respResult = ret ? inst->requestHighIOFreq(p.scene(), p.action(), p.level(), p.timeoutms(), pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_HIGH_IO_FREQ  parse:%d [%d,%d,%d,%d] resp:%d", ret, p.scene(), TOINT(p.action()), p.level(), TOINT(p.timeoutms()), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CANCEL_HIGH_IO_FREQ) {
            int respResult = inst->cancelHighIOFreq(pHead->callertid, pHead->timestamp);
            pdbg("FUNC_CANCEL_HIGH_IO_FREQ resp:%d", respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_SET_SCREEN_RESOLUTION) {
            amc::RequestScreenResolution p;
            int ret = p.ParseFromArray(payload, payloadLen);
            int respResult = ret ? inst->requestScreenResolution(p.level(), p.uiname(), pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_SET_SCREEN_RESOLUTION  parse:%d [%d,%s] resp:%d", ret, p.level(), p.uiname().c_str(), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_RESET_SCREEN_RESOLUTION) {
            int respResult = inst->resetScreenResolution(pHead->callertid, pHead->timestamp);
            pdbg("FUNC_RESET_SCREEN_RESOLUTION resp:%d", respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_REG_SYSTEM_EVENT_CALLBACK) {
            amc::RegisterCallback p;
            int ret = p.ParseFromArray(payload, payloadLen);
            int respResult =  inst->registerSystemEventCallback(p.uid(), pHead->callertid, pHead->timestamp);
            pdbg("FUNC_REG_SYSTEM_EVENT_CALLBACK  parse:%d [%d,%d,%d] resp:%d", ret, p.type(), p.uid(), p.pid(), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_REG_ANR_CALLBACK) {
            amc::RegisterCallback p;
            int ret = p.ParseFromArray(payload, payloadLen);
            int respResult =  inst->registerAnrCallback(p.uid(), pHead->callertid, pHead->timestamp);
            pdbg("FUNC_REG_ANR_CALLBACK  parse:%d [%d,%d,%d] resp:%d", ret, p.type(), p.uid(), p.pid(), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_REG_PRELOAD_BOOT_RESOURCE) {
            amc::RequestBootPreLoadResource p;
            int ret = p.ParseFromArray(payload, payloadLen);
            vector<string> vec;
            if (ret) {
                int size = p.filelist_size();
                for (int i = 0; i < size; i++) {
                    vec.push_back(p.filelist(i));
                }
            }
            int respResult = ret ? inst->registerBootPreloadResource(vec, pHead->callertid, pHead->timestamp) : ERR_PACKAGE_DECODE_FAILED;
            pdbg("FUNC_REG_PRELOAD_BOOT_RESOURCE  parse:%d vecSize:%d resp:%d", ret, TOINT(vec.size()), respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_TERMINATE_APP) {
            int respResult = inst->terminateApp(pHead->callertid, pHead->timestamp);
            pdbg("FUNC_TERMINATE_APP resp:%d", respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_CONFIGURE) {
            amc::Configuration p;
            int ret = p.ParseFromArray(payload, payloadLen);
            int respResult = inst->configure(p.data(), pHead->callertid, pHead->timestamp);
            pdbg("FUNC_CONFIGURE resp:%d", respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        if (pHead->funcid == FUNC_GET_PARAMETERS) {
            amc::GetParameters p;
            int ret = p.ParseFromArray(payload, payloadLen);
            string json = "";
            int respResult = inst->getParameters(p.data(), pHead->callertid, pHead->timestamp, json);
            pdbg("FUNC_GET_PARAMETERS resp:%d", respResult);
            int errCode = respResult >= 0 ? 0 : respResult;
            /***
            ** TODO:
            ** generate the callback data here !!
            ***/
            if (!errCode) {
                amc::DataCallback  dataCallback;
                //type is useless
                dataCallback.set_type(RESPONSE_DATA_TYPE_GETPARAMETER);
                dataCallback.set_data(json);

                int len = dataCallback.ByteSize();
                uint8_t body[len];
                dataCallback.SerializeToArray(body,len);

                genRespPack(pHead->funcid, errCode, pHead->requestid, body, len, &resp, &respLen);
            }
            else
                genRespPack(pHead->funcid, errCode, pHead->requestid, NULL, 0, &resp, &respLen);
            return 0;
        }

        genRespPack(pHead->funcid, ERR_FUNCTION_NOT_SUPPORT, pHead->requestid, NULL, 0, &resp, &respLen);
        return 0;
    }
};

/*Not Used!!, this is for client, and we are server.*/
class IClientEngine {
public:
    virtual bool isRunning() = 0;
    virtual int64_t sendReq(int funcid, uint8_t *data, int dataLen, int tid, int64_t timestamp) = 0;
    virtual  ~IClientEngine(){}
};

class IC2JavaCallback {
public:
    virtual int
    callback(int callbackType, uint64_t timestamp, int errCode, int funcid, int dataType,
             uint8_t *data, int len) = 0;
    virtual ~IC2JavaCallback(){}
};

class ClientProtocal : public IHardCoderDataCallback {

protected:
    virtual IClientEngine *engine()  = 0;

    virtual IC2JavaCallback *c2JavaCallback() =0;


public:
    ClientProtocal() {}

    int checkPermission(int pid, int uid, int tid, int64_t timestamp) {
        if (!engine()) return -3;

        int ret = engine()->sendReq(FUNC_CHECK_PERMISSION, NULL, 0, tid, timestamp);
        pdbg("checkPermission ret:%d pid:%d uid:%d local pid: %d, uid:%d tid:%d timestamp:%d", ret,
             pid, uid, getpid(), getuid(), tid, TOINT(timestamp / 1000000L));
        return ret;
    }

    int requestCpuHighFreq(int scene, int64_t action, int level, int timeoutms, int tid, int64_t timestamp)  {
        if (!engine()) return -3;

        amc::RequestCPUHighFreq request;
        request.set_scene(scene);
        request.set_level(level);
        request.set_timeoutms(timeoutms);
        request.set_action(action);
        uint32_t len = request.ByteSize();
        uint8_t body[len];
        request.SerializeToArray(body, len);

        int ret = engine()->sendReq(FUNC_CPU_HIGH_FREQ, body, len, tid, timestamp);
        pdbg("requestCpuHighFreq ret:%d scene:%d action:%d level:%d timeoutms:%d tid:%d timestamp:%d" ,
             ret, scene, TOINT(action), level, timeoutms, tid, TOINT(timestamp/1000000L));
        return ret;
    }

    int cancelCpuHighFreq(int tid, int64_t timestamp) {
        if (!engine()) return -3;

        int ret = engine()->sendReq(FUNC_CANCEL_CPU_HIGH_FREQ, NULL, 0, tid, timestamp);
        pdbg("cancelCpuHighFreq ret:%d tid:%d timestamp:%d", ret, tid, TOINT(timestamp / 1000000L));
        return ret;
    }

    int requestCpuCoreForThread(int scene, int64_t action, int* bindtids , int bindlen, int timeoutms, int tid, int64_t timestamp) {
        if (!engine()) return -3;

        amc::RequestCPUCoreForThread request;
        request.set_scene(scene);
        for (int index = 0; index < bindlen; index++) {
            request.add_bindtids(bindtids[index]);
        }
        request.set_timeoutms(timeoutms);
        request.set_action(action);
        uint32_t len = request.ByteSize();
        uint8_t body[len];
        request.SerializeToArray(body, len);

        int ret = engine()->sendReq(FUNC_CPU_CORE_FOR_THREAD, body, len, tid, timestamp);
        pdbg("requestCpuCoreForThread ret:%d :%d %d %d %d  ", ret, scene, TOINT(action), bindlen, timeoutms);
        return ret;
    }

    int cancelCpuCoreForThread(int* bindtids , int bindlen, int tid, int64_t timestamp) {
        if (!engine()) return -3;

        if (bindlen <= 0 || bindtids == NULL) {
            perr("cancelCpuCoreForThread bindlen :%d", bindlen);
            return -2;
        }
        amc::CancelCPUCoreForThread request;
        for (int index = 0; index < bindlen; index++) {
            request.add_bindtids(bindtids[index]);
        }
        uint32_t len = request.ByteSize();
        uint8_t body[len];
        request.SerializeToArray(body, len);

        int ret = engine()->sendReq(FUNC_CANCEL_CPU_CORE_FOR_THREAD, body, len, tid, timestamp);
        pdbg("cancelCpuCoreForThread :%d end ret:%d", bindlen, ret);
        return ret;
    }

    int requestHighIOFreq(int scene, int64_t action, int level, int timeoutms, int tid, int64_t timestamp){
        if (!engine()) return -3;

        amc::RequestHighIOFreq request;
        request.set_scene(scene);
        request.set_level(level);
        request.set_timeoutms(timeoutms);
        request.set_action(action);
        uint32_t len = request.ByteSize();
        uint8_t body[len];
        request.SerializeToArray(body, len);

        int ret = engine()->sendReq(FUNC_HIGH_IO_FREQ, body, len, tid, timestamp);
        pdbg("requestCpuCoreForThread :%d %d %d %d end ret:%d", scene, TOINT(action), level, timeoutms, ret);
        return ret;

    }

    int cancelHighIOFreq(int tid, uint64_t timestamp) {
        if (!engine()) return -3;

        int ret = engine()->sendReq(FUNC_CANCEL_HIGH_IO_FREQ, NULL, 0, tid, timestamp);
        pdbg("cancelHighIOFreq ret:%d ", ret);
        return ret;
    }

    int requestScreenResolution(int level, string uiName, int tid, int64_t timestamp){
        if (!engine()) return -3;

        amc::RequestScreenResolution request;
        request.set_level(level);
        request.set_uiname(uiName);
        uint32_t len = request.ByteSize();
        uint8_t body[len];
        request.SerializeToArray(body, len);
        int ret = engine()->sendReq(FUNC_SET_SCREEN_RESOLUTION, body, len, tid, timestamp);
        pdbg("requestScreenResolution level: %d uiName:%s end ret:%d", level, uiName.c_str(), ret);
        return ret;
    }

    int resetScreenResolution(int tid, int64_t timestamp) {
        if (!engine()) return -3;

        int ret = engine()->sendReq(FUNC_RESET_SCREEN_RESOLUTION, NULL, 0, tid, timestamp);
        pdbg("resetScreenResolution end ret:%d ", ret);
        return ret;
    }

    int registerANRCallback(int tid, int64_t timestamp){
        if (!engine()) return -3;

        int ret = engine()->sendReq(FUNC_REG_ANR_CALLBACK, NULL, 0, tid, timestamp);
        pdbg("registerANRCallback end ret:%d ", ret);
        return ret;
    }

    int registerBootPreloadResource(vector<string> files, int tid, int64_t timestamp){
        if (!engine()) return -3;

        amc::RequestBootPreLoadResource request;
        int count = files.size();
        for (int index = 0; index < count; index++) {
            request.add_filelist(files[index]);
        }
        uint32_t len = request.ByteSize();
        uint8_t body[len];
        request.SerializeToArray(body, len);
        int ret = engine()->sendReq(FUNC_REG_PRELOAD_BOOT_RESOURCE, body, len, tid, timestamp);
        pdbg("registerBootPreloadResource files size:%d end ret:%d", TOINT(files.size()), ret);
        return 0;
    }

    int terminateApp(int tid, int64_t timestamp){
        if (!engine()) return -3;

        int ret = engine()->sendReq(FUNC_TERMINATE_APP, NULL, 0, tid, timestamp);
        pdbg("terminateApp ret:%d ", ret);
        return ret;
    }

    int requestUnifyCpuIOThreadCore(int scene, int64_t action, int cpulevel, int iolevel, int* bindtids , int bindlen, int timeoutms, int tid, int64_t timestamp) {
        if (!engine()) return -3;

        amc::RequestUnifyCPUIOThreadCore request;
        request.set_scene(scene);
        request.set_action(action);
        request.set_cpulevel(cpulevel);
        request.set_iolevel(iolevel);
        if (bindlen > 0 && bindtids != NULL) {
            for (int index = 0; index < bindlen; index++) {
                request.add_bindtids(bindtids[index]);
            }
        }
        request.set_timeoutms(timeoutms);
        uint32_t len = request.ByteSize();
        uint8_t body[len];
        request.SerializeToArray(body, len);

        int ret = engine()->sendReq(FUNC_UNIFY_CPU_IO_THREAD_CORE_GPU, body, len, tid, timestamp);
        pdbg("requestUnifyCpuIOThreadCore :%d %d %d %d %d %d end ret:%d", scene, TOINT(action), cpulevel, iolevel, bindlen, timeoutms, ret);
        return ret;
    }

    int cancelUnifyCpuIOThreadCore(int cancelcpu, int cancelio, int cancelthread, int* bindtids , int bindlen, int tid, int64_t timestamp) {
        if (!engine()) return -3;

        amc::CancelUnifyCPUIOThreadCore request;
        request.set_cancelcpu(cancelcpu);
        request.set_cancelio(cancelio);
        request.set_cancelthread(cancelthread);
        if (bindlen > 0 && bindtids != NULL) {
            for (int index = 0; index < bindlen; index++) {
                request.add_bindtids(bindtids[index]);
            }
        }
        uint32_t len = request.ByteSize();
        uint8_t body[len];
        request.SerializeToArray(body, len);
        int ret = engine()->sendReq(FUNC_CANCEL_UNIFY_CPU_IO_THREAD_CORE_GPU, body, len, tid, timestamp);
        pdbg("cancelUnifyCpuIOThreadCore :%d %d %d %d end ret:%d",  cancelcpu, cancelio, cancelthread, bindlen, ret);
        return ret;
    }

    int onCallback(int uid, int funcid, uint64_t requestid, uint64_t timestamp, int retCode,
                   uint8_t *data, int len) {
        UNUSED(uid);
        UNUSED(requestid);

        if(!c2JavaCallback()){
            pdbg("onCallback c2JavaCallback:0x%x funcid:%d data:0x%x len:%d",
                 TOINT(c2JavaCallback()), funcid, TOINT(data), len);
            return 0;
        }

        if (retCode) {
            c2JavaCallback()->callback(1, timestamp, retCode, funcid, 0, NULL, 0);
            return 0;
        }

        if ((funcid != FUNC_REG_ANR_CALLBACK) || len <= 0 || (!data)) {
            pdbg("onCallback c2JavaCallback:0x%x funcid:%d data:0x%x len:%d",
                 TOINT(c2JavaCallback()), funcid, TOINT(data), len);
            return 0;
        }

        amc::DataCallback p;
        int ret = p.ParseFromArray(data, len);
        if (!ret) {
            pdbg("onCallback ParseFromArray ret:%d c2JavaCallback:0x%x funcid:%d data:0x%x len:%d",
                 ret, TOINT(c2JavaCallback()), funcid, TOINT(data), len);
            return 0;
        }

        c2JavaCallback()->callback(2, 0, 0, funcid, p.type(), (uint8_t *) p.data().data(),
                                   p.data().size());
        return 0;
    }
};


#endif //AMC_PROTOCOL_H

