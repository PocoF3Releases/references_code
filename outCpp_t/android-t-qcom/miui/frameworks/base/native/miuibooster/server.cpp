//
// server function implement
// Created by dk on 17/4/8.
//

#include "miuibooster.h"
#include <pthread.h>
#include "json/json.h"
#include "server.h"

using namespace std;
using namespace amc;

#include "MiuiBoosterService.h"
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
using namespace android;

const string testANRMessage = "this is an anr test log message from server!";
const int testSystemEventCode = 12345;

#define TOPAPP_BOOSTER_SOCKET_NAME "miui_booster"

class ManufacturerCoder : public HardCoder {
public:

    int getUidByAddress(const char *ip, int port) {
        UNUSED(ip);
        return LocalPortCheck::getUidByPort(LocalPortCheck::UDP_PORT, port);
    }

    bool checkPermission(int pid, int uid, int callertid, int64_t timestamp) {
        pdbg("ManufacturerCoder checkPermission, pid:%d pid:%d callertid:%d timestamp:%d",
            pid, uid, callertid, TOINT(timestamp/1000000L));
        return true;
    }

    int requestCpuHighFreq(int scene, int64_t action, int level,
        int timeoutms, int callertid, int64_t timestamp) {
        return miui_booster.requestCpuHighFreq(scene, action, level, timeoutms, callertid, timestamp);
    }

    int cancelCpuHighFreq(int callertid, int64_t timestamp) {
        return miui_booster.cancelCpuHighFreq(callertid,timestamp);
    }

    int requestGpuHighFreq(int scene, int64_t action, int level,
        int timeoutms, int callertid, int64_t timestamp) {
        //return ERR_FUNCTION_NOT_SUPPORT;
        pdbg("requestGpuHighFreq starting!");
        return miui_booster.requestGpuHighFreq(scene, action, level, timeoutms, callertid, timestamp);
    }

    int cancelGpuHighFreq(int callertid, int64_t timestamp) {
        pdbg("ManufacturerCoder cancelGpuHighFreq, callertid:%d timestamp:%d",
            callertid, TOINT(timestamp/1000000L));
        return RET_OK;
    }

    int requestThreadPriority(int scene, int64_t action, vector<int> bindtids,
        int timeoutms, int callertid, int64_t timestamp) {
        return miui_booster.requestThreadPriority(scene, action, bindtids,timeoutms, callertid, timestamp);
    }

    int cancelThreadPriority(vector<int> bindtids, int callertid, int64_t timestamp) {
        return miui_booster.cancelThreadPriority(bindtids, callertid, timestamp);
    }

    int requestHighIOFreq(int scene, int64_t action, int level, int timeoutms, int callertid, int64_t timestamp) {
        pdbg("ManufacturerCoder requestHighIOFreq, scene:%d, action:%d, level:%d, timeoutms:%d, callertid:%d timestamp:%d",
            scene, TOINT(action), level, timeoutms,
            callertid, TOINT(timestamp/1000000L));
        return ERR_FUNCTION_NOT_SUPPORT;
    }

    int cancelHighIOFreq(int callertid, int64_t timestamp) {
        pdbg("ManufacturerCoder cancelHighIOFreq, callertid:%d timestamp:%d",
            callertid, TOINT(timestamp/1000000L));
        return RET_OK;
    }

    int requestScreenResolution(int level, string uiName, int callertid, int64_t timestamp) {
        pdbg("ManufacturerCoder requestScreenResolution, level::%d uiName:%s callertid:%d timestamp:%d",
            level, uiName.c_str(), callertid, TOINT(timestamp/1000000L));
        return RET_OK;
    }

    int resetScreenResolution(int callertid, int64_t timestamp) {
        pdbg("ManufacturerCoder resetScreenResolution, callertid:%d timestamp:%d",
            callertid, TOINT(timestamp/1000000L));
        return RET_OK;
    }

    int terminateApp(int callertid, int64_t timestamp) {
        pdbg("ManufacturerCoder terminateApp, callertid:%d timestamp:%d",
            callertid, TOINT(timestamp/1000000L));
        return RET_OK;
    }

    int requestUnifyCpuIOThreadCoreGpu(int scene, int64_t action, int cpulevel, int iolevel,
        vector<int>bindtids, int gpulevel, int timeoutms, int callertid, int64_t timestamp) {
        return miui_booster.requestUnifyCpuIOThreadCoreGpu(scene, action, cpulevel, iolevel,
                                    bindtids, gpulevel, timeoutms, callertid, timestamp);
    }

    int cancelUnifyCpuIOThreadCoreGpu(int cancelcpu, int cancelio, int cancelthread, vector<int>bindtids,
        int cancelgpu, int callertid, int64_t timestamp) {
        return miui_booster.cancelUnifyCpuIOThreadCoreGpu(cancelcpu, cancelio, cancelthread, bindtids,
                                                          cancelgpu, callertid, timestamp);
    }

    int configure(const string& data, int callertid, int64_t timestamp) {
        UNUSED(data);
        UNUSED(callertid);
        UNUSED(timestamp);
        pdbg("ManufacturerCoder configure");
        return RET_OK;
    }

    //Taobao didn't use this function, so just implement here ,not in miuibooster.cpp
    //return miui_booster.getParameters(data, callertid, timestamp);
    int getParameters(const string& data, int callertid, int64_t timestamp, string& respData) {
        pdbg("MiuiBoosterService getParameters, data %s callertid:%d timestamp:%d respData:%s",
                 data.c_str(), callertid, TOINT(timestamp/1000000L), respData.c_str());


        //data string ,it's a json string, like this :
        // {"getparameterstype":1,"getparameterskeys":["HCMinQPKey","HCMaxQPKey","HCQPSceneKey"]}
        if(data.find("HCMinQPKey") == string::npos && data.find("HCMaxQPKey") == string::npos){
            // reserve , now we only deal with QP range set.
            return -1;
        }

        //Fill response data here.
        Json::Value  valueinfo;
        string jsonStr;
        Json::StreamWriterBuilder writerBuilder;
        ostringstream os;

        valueinfo["getparameterstype"] = GETPARAMETERS_TYPE_QP_MEDIACODEC;

        valueinfo["HCMinQPKey"] = "range-qp-min";
        valueinfo["HCMaxQPKey"] = "range-qp-max";
        valueinfo["HCQPSceneKey"] = "null";

        std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
        jsonWriter->write(valueinfo, &os);
        jsonStr = os.str();

        pdbg("MiuiBoosterService getParameters END, respData:%s", jsonStr.c_str());

        return RET_OK;
    }

    int registerAnrCallback(int uid, int callertid, int64_t timestamp) {
        list<int>& cbL = this->anrCallbackUidList;
        pdbg("ManufacturerCoder registerAnrCallback, anrCallbackList size:%d callertid:%d timestamp:%d",
            TOINT(cbL.size()), callertid, TOINT(timestamp/1000000L));

        if (!(cbL.empty()) && (find(cbL.begin(), cbL.end(), uid) !=  cbL.end())){
            pdbg("ManufacturerCoder has registered anr before uid:%d", uid)
        }else {
            this->anrCallbackUidList.push_back(uid);
            pdbg("ManufacturerCoder registerAnrCallback uid:%d", uid);
        }

        return RET_OK;
    }

    int registerSystemEventCallback(int uid, int callertid, int64_t timestamp) {
        list<int>& cbL = this->systemEventCallbackList;
        pdbg("ManufacturerCoder registerSystemEventCallback systemEventCallbackList size:%d callertid:%d timestamp:%d", TOINT(cbL.size()), callertid, TOINT(timestamp/1000000L));

        if (!(cbL.empty()) && (find(cbL.begin(), cbL.end(), uid) !=  cbL.end())){
            pdbg("ManufacturerCoder has registered system event callback before uid:%d", uid)
        }else {
            this->systemEventCallbackList.push_back(uid);
            pdbg("ManufacturerCoder registerSystemEventCallback uid:%d size:%d", uid, cbL.size());
        }

        property_set("sys.hardcoder.registered", "true");

        return RET_OK;
    }

    int registerBootPreloadResource(vector<string> file, int callertid, int64_t timestamp) {
        pdbg("ManufacturerCoder registerBootPreloadResource, file size:%d callertid:%d timestamp:%d",
            TOINT(file.size()), callertid, TOINT(timestamp));

        return RET_OK;
    }

    const char* eventToJson(int eventCode)
    {
        Json::Value  writevalueinfo;
        string jsonStr;
        Json::StreamWriterBuilder writerBuilder;
        ostringstream os;

        writevalueinfo[SYSTEM_EVENT_CODE] = eventCode;
        std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
        jsonWriter->write(writevalueinfo, &os);
        jsonStr = os.str();
        const char *str = jsonStr.c_str();

        return str;
    }

    // to be invoked by system on event
    void onEvent(int eventCode){
        pdbg("callbackThread event:%d DataToJson:%s", eventCode, eventToJson(eventCode));
        amc::DataCallback  dataCallback;
        dataCallback.set_type(REGISTER_CALLBACK_TYPE_SYSTEM_EVENT);
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%s", eventToJson(eventCode));
        buffer[sizeof(buffer)-1] = '\0';
        dataCallback.set_data(buffer);
        cbQueue.push(dataCallback);
    }

    // to be invoked by system on anr
    void onANR(string log){
        pdbg("callbackThread log:%s", log.c_str());
        amc::DataCallback  dataCallback;
        dataCallback.set_data(log);
        dataCallback.set_type(REGISTER_CALLBACK_TYPE_ANR);
        cbQueue.push(dataCallback);
    }

    ManufacturerCoder(IHardCoderDataCallback* callback) {
        this->callback = callback;
        pthread_t tid;
        pthread_create(&tid, NULL, callbackThread, this);
    }
    ~ManufacturerCoder() {
        this->anrCallbackUidList.clear();
        this->systemEventCallbackList.clear();
        callback = NULL;
    }

    static void *callbackThread(void *args) {
        pdbg("callbackThread run pid:%d tid:%d", getpid(), gettid());
        ManufacturerCoder* coder = (ManufacturerCoder *) args;
        while (1) {
            IHardCoderDataCallback *callback = coder->callback;
            amc::DataCallback  dataCallback = coder->cbQueue.pop();
            int type = dataCallback.type();

            if (type == REGISTER_CALLBACK_TYPE_SYSTEM_EVENT){
                list<int> cbl = coder->systemEventCallbackList;
                if (cbl.empty()){
                    pdbg("systemEventCallbackList is empty size:%d continue", TOINT(cbl.size()));
                    continue;
                }
                list<int>::iterator it;
                for(it = cbl.begin(); it != cbl.end(); ++it){
                    int len = dataCallback.ByteSize();
                    uint8_t body[len] ;
                    dataCallback.SerializeToArray(body,len);
                    callback->onCallback(*it, FUNC_REG_SYSTEM_EVENT_CALLBACK, 0, 0,0 , body, len);
                    pdbg("system event callback done for succ uid:%d", *it);
                }

            } else if(type == REGISTER_CALLBACK_TYPE_ANR) {
                list<int> cbl = coder->anrCallbackUidList;
                if (cbl.empty()) {
                    pdbg("anrCallbackUidList is empty size:%d continue", TOINT(cbl.size()));
                    continue;
                }
                list<int>::iterator it;
                for (it = cbl.begin(); it != cbl.end(); ++it) {
                    int len = dataCallback.ByteSize();
                    uint8_t body[len];
                    dataCallback.SerializeToArray(body, len);
                    callback->onCallback(*it, FUNC_REG_ANR_CALLBACK, 0, 0, 0, body, len);
                    pdbg("anr callback done for succ uid:%d", *it);
                }
            }
        }
        pwrn("callbackThread loop QUIT now.");
        return NULL;
    }

    //for test system event
    static void* testSystemEvent(void *args) {
        sleep(10);
        ManufacturerCoder *coder = (ManufacturerCoder *) args;
        coder->onEvent(testSystemEventCode);
        /**
        IHardCoderDataCallback *callback = coder->callback;
        list<int>& cbL = coder->systemEventCallbackList;
        if (cbL.empty()){
            pdbg("testSystemEvent but systemEventCallbackList is empty size:%d", cbL.size());
            return NULL;
        }

        amc::DataCallback  dataCallback;
        dataCallback.set_type(1);
        dataCallback.set_data(testSystemEvent);

        int len = dataCallback.ByteSize();
        uint8_t body[len];
        dataCallback.SerializeToArray(body,len);
        pdbg("testSystemEvent systemEventCallbackList size:%d %d", cbL.size());
        int64_t ret = callback->onCallback(cbL.front(), FUNC_REG_SYSTEM_EVENT_CALLBACK, body, len);
         **/
        pdbg("testSystemEvent exit tid:%d", gettid());
        return NULL;
    }

    // for test anr
    static void *testANR(void *args) {
        sleep(10);
        ManufacturerCoder* coder = (ManufacturerCoder *) args;
        coder->onANR(testANRMessage);
//        IHardCoderDataCallback *callback = coder->callback;
//        list<int>& cbL = coder->anrCallbackList;
//        if (cbL.empty()){
//            pdbg("testANR but anrCallbackList is empty size:%d", cbL.size());
//            return NULL;
//        }
//
//        amc::DataCallback  dataCallback;
//        dataCallback.set_anr(testANRMessage);
//
//        int len = dataCallback.ByteSize();
//        uint8_t body[len];
//        dataCallback.SerializeToArray(body,len);
//        pdbg("testANR anrCallbackList size:%d %d", cbL.size());
//        int64_t ret = callback->onCallback(cbL.front(), FUNC_REG_ANR_CALLBACK, body, len);
        pdbg("testANR exit tid:%d", gettid());
        return NULL;
    }

private:
    list<int> anrCallbackUidList;
    list<int> systemEventCallbackList;
    IHardCoderDataCallback* callback;
    rqueue<amc::DataCallback, 0> cbQueue;
    MiuiBoosterUtils miui_booster;
};

int is_device_miui_booster_supported() {
    int ret = -1;
    char value[PROP_VALUE_MAX];
    property_get("persist.sys.enable_miui_booster", value, "1");
    return strcmp(value, "1") ? 0 : 1;
}

void *socket_thread(void *x) {
   UNUSED(x);
   LocalSocketServer localsocket;
   ManufacturerCoder *hc = new ManufacturerCoder((IHardCoderDataCallback*) &localsocket);
   localsocket.start(TOPAPP_BOOSTER_SOCKET_NAME, hc);
   return NULL;
}


void *binder_thread(void *x) {
   UNUSED(x);
   pwrn("Miuibooster service is starting.");
   android::sp<android::ProcessState> ps(android::ProcessState::self());
   ps->setThreadPoolMaxThreadCount(4);
   ps->startThreadPool();
   ps->giveThreadPoolName();
   android::sp<android::IServiceManager> sm(android::defaultServiceManager());
   const android::status_t service_status =
         sm->addService(android::String16("miuiboosterservice"),
                        new MiuiBoosterService);
   if (service_status != android::OK) {
       pdbg("MiuiBoosterService not added: %d",
             static_cast<int>(service_status));
   } else{
       pdbg("MiuiBoosterService added: %d",
                static_cast<int>(service_status));
       android::IPCThreadState::self()->joinThreadPool();
   }
   return NULL;
}


int main() {
   setTag("MiuiBoosterServer");
   if (is_device_miui_booster_supported() == 0){
       pwrn("Miuibooster not enabled!!!");
       return 0;
   }
   pthread_t socket_t, binder_t;
   pwrn("Miuibooster server start!!!");
   pthread_create(&socket_t,
          NULL,
          socket_thread,
          NULL);
   pthread_create(&binder_t,
          NULL,
          binder_thread,
          NULL);
   // start socket
   pthread_join(socket_t, NULL);

   // start binder service
   pthread_join(binder_t, NULL);

   return 0;
}

