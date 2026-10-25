// 20220707 created by gj

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <map>
#include <set>
#include <stdlib.h>
#include <fcntl.h>
#include <mutex>
#include <algorithm>
#include <signal.h>
#include <pthread.h>
#include <nativehelper/JNIHelp.h>
#include <android-base/stringprintf.h>
#include "utils.h"
#include "Tcping.h"

#define LOG_TAG "NetworkAccelerateSwitch"
// TODO 是否需要LOG_NDEBUG 调试阶段结束可以把这个去掉
#define LOG_NDEBUG 0
#include <cutils/log.h>

#define SERVER_BUFF_SIZE 16
#define DUMP_INDENT "    "

#define RTT_THREAD_MIN_POLL_TIME 1000
#define RTT_THREAD_EXPECTED_ROLL_TIME 1500

using android::base::StringPrintf;

typedef enum {
    WAIT_TO_CREATE = 0,
    USAGE,
    WAIT_TO_DESTROY,
} ThreadStatus;

// rtt server ip and prot
typedef struct _TcpInfo {
    char server[SERVER_BUFF_SIZE];
    int port;
    bool operator==(const struct _TcpInfo &o) const {
        return port == o.port && strcmp(server, o.server) == 0;
    }
    bool operator<(const struct _TcpInfo &o) const{
        int cmp = strcmp(server, o.server);
        return cmp < 0 || cmp == 0 && port < o.port;
    }
} TcpInfo;

typedef struct {// add pthread_t
    int rtt;
    std::string interface_name;
    TcpInfo tcp_info = {DEFAULT_RTT_SERVER, DEFAULT_RTT_PORT};
} RttResult;

typedef struct _BlackTarget {
    TcpInfo tcp_info;
    struct timespec ts;
    std::string report_interfaces;

    _BlackTarget() {
        tcp_info = {DEFAULT_RTT_SERVER, DEFAULT_RTT_PORT};
        clock_gettime( CLOCK_REALTIME, &ts);
    }

    bool operator==(const struct _BlackTarget &o) const {
        return tcp_info == o.tcp_info;
    }

    bool operator<(const struct _BlackTarget &o) const {
        return tcp_info < o.tcp_info;
    }
} BlackTarget;

#define TCP_INFO_SIZE (3)
#define TCP_TARTGET_INDEX (0)
#define UDP_TARTGET_INDEX (1)
#define FALLBACK_TARTGET_INDEX (TCP_INFO_SIZE - 1)
#define BLACK_TARGET_HISTROY_SIZE 30
#define INVALID_TARTGET_JUDGEMENT 3

// static TcpInfo tcpinfo = {DEFAULT_RTT_SERVER, DEFAULT_RTT_PORT};
static TcpInfo s_tcp_info[TCP_INFO_SIZE] = {
    {DEFAULT_RTT_SERVER, DEFAULT_RTT_PORT}, // tcp
    {DEFAULT_RTT_SERVER, DEFAULT_RTT_PORT}, // udp
    {DEFAULT_RTT_SERVER, DEFAULT_RTT_PORT} // fallback
};
static int tcp_info_index = FALLBACK_TARTGET_INDEX;

typedef struct _RttThread{
    pthread_t id; // pthread_self()
    std::string interface_name;
    ThreadStatus status;

    _RttThread () {
        id = 0;
        interface_name = "";
        status = WAIT_TO_CREATE;
    }

    bool operator==(const struct _RttThread &o){
        return id == o.id;
    }
    
    // bool operator<(const RttThread &o){
    //     return id < o.id;
    // }
} RttThread;

/**
 * rtt manager thread vars begin
 */
static pthread_t rtt_manager_thread = 0;
static std::mutex rtt_manager_lock;
static std::condition_variable rtt_manager_cv;
// thread controllers
static bool stop_collection = true;
static bool quit_threads = false;
// TODO 如有必要增加一个command queue.
static std::vector<RttThread*> rtt_collection_threads;
/** rtt manager thread vars end */

/**
 * rtt collection thread vars begin
 */
static std::mutex rtt_collection_lock;
static std::condition_variable rtt_collection_cv;
static std::vector<RttResult*> rtt_results;
static std::vector<BlackTarget*> target_black_list;
/** rtt collection thread vars end */

//
static jobject mServiceObj = NULL;

static struct {
    jclass clazz;
    jmethodID notifyRttInfo;
} mServiceClassInfo;

static struct {
    jclass clazz;
    jmethodID constructor;
} mRttResultClassInfo;

static jstring nativeDump(JNIEnv* env, jclass /* clazz */) {
    std::string dump = DUMP_INDENT"NetworkAccelerateSwitch native status:";
    // do dump
    dump += DUMP_INDENT "NetworkAccelerateSwitchService native dump begin:\n";
    { // acquire lock
        std::unique_lock<std::mutex> lock_thread(rtt_manager_lock);
        dump += StringPrintf(DUMP_INDENT "rtt_manager_thread: %ld\n", rtt_manager_thread);
        dump += StringPrintf(DUMP_INDENT "stop_collection: %s\n", (stop_collection ? "true" : "false"));
        dump += StringPrintf(DUMP_INDENT "quit_threads: %s\n", (quit_threads ? "true" : "false"));
        for (std::vector<RttThread*>::iterator it = rtt_collection_threads.begin(); it != rtt_collection_threads.end(); it++) {
            dump += StringPrintf(DUMP_INDENT "RttThread<id:%ld interface_name:%s status:%d>\n", (*it)->id, (*it)->interface_name.c_str(), (*it)->status);
        }
        // dump += StringPrintf(DUMP_INDENT "tcpinfo: %s:%d\n", tcpinfo.server, tcpinfo.port);
        for (int i = 0; i < TCP_INFO_SIZE; i++) {
            dump += StringPrintf(DUMP_INDENT "s_tcp_info[%d]: %s:%d\n", i, s_tcp_info[i].server, s_tcp_info[i].port);
        }
        dump += StringPrintf(DUMP_INDENT "tcp_info_index: %d\n", tcp_info_index);
    } // release lock
    { // aquire rtt_collection_lock
        std::unique_lock<std::mutex> lk_for_rtt_thread(rtt_collection_lock);
        dump += StringPrintf(DUMP_INDENT "target_black_list: %zu\n", target_black_list.size());
        for (std::vector<BlackTarget*>::iterator iter = target_black_list.begin(); iter != target_black_list.end(); iter++) {
            clock_gettime( CLOCK_REALTIME, &((*iter)->ts));
            struct tm * timeinfo = localtime(&((*iter)->ts).tv_sec);
            dump += StringPrintf(DUMP_INDENT DUMP_INDENT "<%s:%d interfaces:%s date:%.2d-%.2d %.2d:%.2d:%.2d.%.3ld>\n"
                    , (*iter)->tcp_info.server, (*iter)->tcp_info.port, (*iter)->report_interfaces.c_str(), timeinfo->tm_mon + 1,
                    timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, (*iter)->ts.tv_nsec / 1000000);
        }
    } // release rtt_collection_lock
    dump += DUMP_INDENT "end\n";
    return env->NewStringUTF(dump.c_str());
}

static void clearTcpInfo() {
    for (int i = 0; i < TCP_INFO_SIZE; i++) {
        strcpy(s_tcp_info[i].server, DEFAULT_RTT_SERVER);
        s_tcp_info[i].port = DEFAULT_RTT_PORT;
    }
    tcp_info_index = FALLBACK_TARTGET_INDEX;
}

static jint stopRttCollection(JNIEnv* /*env*/, jobject /*object*/) {
    ALOGI("TODO do stop thread");
    { // acquire lock
        std::unique_lock<std::mutex> lock_thread(rtt_manager_lock);
        for (std::vector<RttThread*>::iterator it = rtt_collection_threads.begin(); it != rtt_collection_threads.end();) {
            switch ((*it)->status) {
                case WAIT_TO_CREATE:
                    delete (*it);
                    it = rtt_collection_threads.erase(it);
                    break;
                case USAGE:
                    (*it)->status = WAIT_TO_DESTROY;
                    /* FALLTHROUGH */
                case WAIT_TO_DESTROY:
                    it++;
                    break;
                default:
                    ALOGE("stopRttCollection unknown status: %d", (*it)->status);
                    it++;
                    break;
            }
        }
        stop_collection = true;
    } // release lock

    // strcpy(tcpinfo.server, DEFAULT_RTT_SERVER);
    // tcpinfo.port = DEFAULT_RTT_PORT;
    clearTcpInfo();
    { // aquire rtt_collection_lock
        std::unique_lock<std::mutex> lk_for_rtt_thread(rtt_collection_lock);
        for (std::vector<BlackTarget*>::iterator iter = target_black_list.begin(); iter != target_black_list.end();) {
            delete (*iter);
            iter = target_black_list.erase(iter);
        }
    } // release rtt_collection_lock

    return 0;
}

static void* doRttCollection(void *para) {
    RttThread *rttthead = (RttThread*) para;// guard by rtt_manager_lock
    Ping_info ping_info;
    memset(&ping_info, 0, sizeof(Ping_info));
    while (1)
    {
        // {
        //     std::unique_lock<std::mutex> lk_for_rtt_thread(rtt_collection_lock);
        //     rtt_thread_cv.wait(lk_for_rtt_thread);
        // }

        { // acquire rtt_manager_lock
            std::unique_lock<std::mutex> lock_thread(rtt_manager_lock);
            bool found_self = false;
            for (std::vector<RttThread*>::iterator it = rtt_collection_threads.begin(); it != rtt_collection_threads.end(); it++) {
                if (rttthead != *it) {
                    continue;
                }
                found_self = true;
                if (rttthead->id != pthread_self()) {
                    ALOGE("doRttCollection(%d) pthread_self:%ld is different from id:%ld! to do recovery", gettid(), pthread_self(), rttthead->id);
                    rttthead->id = pthread_self();
                }
                if (rttthead->status == WAIT_TO_DESTROY) {
                    rtt_collection_threads.erase(it);
                    goto rtt_colection_exit;
                }
            }
            if (!found_self) {
                ALOGE("doRttCollection(%d) id:%ld! not found in list:", gettid(), rttthead->id);
                for (std::vector<RttThread*>::iterator it = rtt_collection_threads.begin(); it != rtt_collection_threads.end(); it++) {
                    ALOGE("RttThread<id:%ld interface_name:%s status:%d>", (*it)->id, (*it)->interface_name.c_str(), (*it)->status);
                }
                goto rtt_colection_exit;
            }
            ping_info.device = rttthead->interface_name.c_str();
        } // release rtt_manager_lock

        RttResult* p_rtt_result = new RttResult();
        p_rtt_result->interface_name = std::string(ping_info.device);
        strcpy(p_rtt_result->tcp_info.server, s_tcp_info[tcp_info_index].server);
        p_rtt_result->tcp_info.port = s_tcp_info[tcp_info_index].port;
        ping_info.server = /*tcpinfo*/p_rtt_result->tcp_info.server;
        ping_info.port = /*tcpinfo*/p_rtt_result->tcp_info.port;
        p_rtt_result->rtt = getInterfaceTcpingRtt(&ping_info);

        { // acquire rtt_collection_lock
            std::unique_lock<std::mutex> lk_for_rtt_thread(rtt_collection_lock);
            rtt_results.push_back(p_rtt_result);
            rtt_collection_cv.wait(lk_for_rtt_thread);
        } // release rtt_collection_lock

    }

rtt_colection_exit:
    ALOGI("doRttCollection(%d):%ld exit.", gettid(), pthread_self());
    delete rttthead; // make sure rttthread is not belong to rtt_manager_thread
    return NULL;
}

static jint startRttCollection(JNIEnv* env, jobject /*object*/, jstring interfaces) {
    std::string str_interfaces;
    jstring2string(env, interfaces, str_interfaces);
    ALOGI("startRttCollection test: %s", str_interfaces.c_str());
    std::vector<std::string> vec_interfaces;
    stringSplit(str_interfaces, ',', vec_interfaces);
    for(std::vector<std::string>::iterator it = vec_interfaces.begin(); it != vec_interfaces.end(); it++) {
        ALOGI("startRttCollection vec_interfaces: %s", it->c_str());
    }
    { // acquire lock
        std::unique_lock<std::mutex> lock_thread(rtt_manager_lock);
        
        // modify current rtt collection threads info
        for (std::vector<RttThread*>::iterator it = rtt_collection_threads.begin(); it != rtt_collection_threads.end(); it++) {
            // check thread exist or not.
            bool already_exist = false;
            for (std::vector<std::string>::iterator iter = vec_interfaces.begin(); iter != vec_interfaces.end();) {
                if (*iter == (*it)->interface_name) {
                    already_exist = true;
                    switch ((*it)->status) {
                        case WAIT_TO_CREATE:
                        case USAGE:
                            // nothing need to do.
                            break;
                        case WAIT_TO_DESTROY:
                            (*it)->status = USAGE;
                            break;
                        default:
                            ALOGE("startRttCollection unknown status: %d", (*it)->status);
                            break;
                    }
                    iter = vec_interfaces.erase(iter);
                    break;
                }
                else {
                    iter++;
                }
            }
            if (!already_exist) {
                // stop thread.
                (*it)->status = WAIT_TO_DESTROY;
            }
        }
        // add new rtt collection threads info
        for (std::vector<std::string>::iterator iter = vec_interfaces.begin(); iter != vec_interfaces.end(); iter++) {
            RttThread* p_new_thread = new RttThread();
            p_new_thread->interface_name = *iter;
            // pthread_attr_t attr;
            // pthread_attr_init(&attr);
            // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            // int ret = pthread_create(&p_new_thread->id, &attr, doRttCollection,
            //                                 (void *)p_new_thread);
            // ALOGI("test......%s %ld", p_new_thread->interface_name.c_str(), p_new_thread->id);
            // if (ret) {
            //     // TODO fail
            //     ALOGE("startRttCollection rtt thread create failed! %d(%s)", errno, strerror(errno));
            //     delete p_new_thread;
            // }
            // else {
            //     p_new_thread->status = USAGE;
                rtt_collection_threads.push_back(p_new_thread);
            // }
        }

        // // check wakeup time
        // auto current_time = std::chrono::steady_clock::now();
        if (stop_collection) {
            stop_collection = false;
            rtt_manager_cv.notify_all();
        }
    } // release lock
    ALOGI("TODO do start thread");
    return 0;
}

static int doNotifyRtt() {
    ALOGI("---doNotifyRtt---");
    int wait_time_ms = RTT_THREAD_EXPECTED_ROLL_TIME;
    int result_size = 0;
    { // attach
        AttachToJavaThread attacher;
        JNIEnv* env = attacher.getEnv();
        if (env == NULL) {
            ALOGE("doNotifyRtt Unable to get JNI env");
            return wait_time_ms;
        }

        if (mServiceObj) {
            jobjectArray array_result = NULL;
            { // aquire rtt_collection_lock
                std::unique_lock<std::mutex> lk_for_rtt_thread(rtt_collection_lock);
                result_size = rtt_results.size();
                if (result_size == 0) {
                    ALOGE("doNotifyRtt result_size is 0");
                    return wait_time_ms;
                }
                array_result = env->NewObjectArray(result_size, mRttResultClassInfo.clazz, NULL);
                if (array_result == NULL) {
                    ALOGE("doNotifyRtt array_result create failed");
                    return wait_time_ms;
                }
                int i = 0;
                // std::map<TcpInfo, std::pair<std::string, int>> check_map;
                std::set<TcpInfo> tcpinfo_set;
                std::set<std::string> interface_set;
                bool all_timeout = true;
                for (std::vector<RttResult*>::iterator it = rtt_results.begin(); it != rtt_results.end(); i++) {
                    jstring if_name = env->NewStringUTF((*it)->interface_name.c_str());
                    if (!if_name) {
                        ALOGE("doNotifyRtt if_name create failed");
                    }
                    jobject obj_result = env->NewObject(mRttResultClassInfo.clazz, mRttResultClassInfo.constructor, mServiceObj, (*it)->rtt, if_name);
                    if (obj_result) {
                        env->SetObjectArrayElement(array_result, i, obj_result);
                        env->DeleteLocalRef(obj_result);
                    }
                    else {
                        ALOGE("doNotifyRtt obj_result create failed");
                    }
                    env->DeleteLocalRef(if_name);

                    // update
                    tcpinfo_set.insert((*it)->tcp_info);
                    interface_set.insert((*it)->interface_name); 
                    if ((*it)->rtt != TIME_OUT_MS) {
                        all_timeout = false;
                    }

                    // remove record
                    delete (*it);
                    it = rtt_results.erase(it);
                }
                // update black list
                if (!all_timeout) {
                    for (std::vector<BlackTarget*>::iterator iter = target_black_list.begin(); iter != target_black_list.end();) {
                        std::set<TcpInfo>::iterator iter_tcpinfo = tcpinfo_set.find((*iter)->tcp_info);
                        if (iter_tcpinfo != tcpinfo_set.end()) {
                            delete (*iter);
                            iter = target_black_list.erase(iter);
                        }
                        else {
                            iter++;
                        }
                    }
                }
                else if (interface_set.size() > 1 && !tcpinfo_set.empty()) {
                    std::string report_interfaces = "";
                    for (std::string interface : interface_set) {
                        report_interfaces.append(interface);
                        report_interfaces.append(",");
                    }
                    report_interfaces.pop_back();// remove last ','
                    for (TcpInfo tcp_info : tcpinfo_set) {
                        if (s_tcp_info[tcp_info_index] == tcp_info) {
                            ALOGE("doNotifyRtt target[%d](%s:%d) in timeout, change to fallback target"
                                    , tcp_info_index, s_tcp_info[tcp_info_index].server, s_tcp_info[tcp_info_index].port);
                            tcp_info_index = FALLBACK_TARTGET_INDEX;
                        }
                        BlackTarget* temp_target = new BlackTarget();
                        temp_target->report_interfaces = report_interfaces;
                        temp_target->tcp_info = tcp_info;
                        target_black_list.push_back(temp_target);
                    }
                    while (target_black_list.size() > BLACK_TARGET_HISTROY_SIZE) {
                        target_black_list.erase(target_black_list.begin());
                    }
                }
            } // release rtt_collection_lock
            jint ret = env->CallIntMethod(mServiceObj, mServiceClassInfo.notifyRttInfo, array_result);
            ALOGI("doNotifyRtt ret = %d", ret);
        }
        else {
            ALOGE("doNotifyRtt mServiceObj is NULL");
        }
    } // detach
    return wait_time_ms;
}

static int cleanExpiredResults() {
    { // aquire rtt_collection_lock
        std::unique_lock<std::mutex> lk_for_rtt_thread(rtt_collection_lock);
        for (std::vector<RttResult*>::iterator it = rtt_results.begin(); it != rtt_results.end(); it++) {
            delete (*it);
        }
        rtt_results.clear();
    } // release rtt_collection_lock
    return RTT_THREAD_EXPECTED_ROLL_TIME;
}

static void* rttManagerThreadLoop(void* /*param*/) {
    ALOGI("rttManagerThreadLoop start");
    int wait_time_ms = RTT_THREAD_MIN_POLL_TIME;
    auto last_notify_time = std::chrono::steady_clock::now();
    while (1) {
        { // acquire lock
            std::unique_lock<std::mutex> lock_thread(rtt_manager_lock);
            // check quit
            if (quit_threads) {
                ALOGI("rttManagerThreadLoop quit!");// TODO log相关参数
                break;
            }

            // check rtt thread status
            std::chrono::milliseconds sleep_time = std::chrono::milliseconds(wait_time_ms);
            auto start_time = std::chrono::steady_clock::now();
            auto d_time = start_time - last_notify_time;
            // long long test_time = d_time;
            // ALOGI("test_time: %lld", test_time);
            if (d_time < sleep_time) {
                rtt_manager_cv.wait_until(lock_thread, start_time + sleep_time - d_time, [&]()->bool{
                    return stop_collection;
                });
            }
            else {
                ALOGE("rttManagerThreadLoop do too much work!!!");
            }

            //
            bool is_expired = false;
            if (stop_collection) {
                ALOGI("rttManagerThreadLoop stop");
                rtt_collection_cv.notify_all();
                auto stopped_time = std::chrono::steady_clock::now();
                rtt_manager_cv.wait(lock_thread);
                auto woken_time = std::chrono::steady_clock::now();
                if (sleep_time < woken_time - stopped_time) {
                    is_expired = true;
                }
            }

            // add new rtt collection threads info
            bool has_new_thread = false;// TODO 确定是否需要因为新线程创建等待一轮结果?
            for (std::vector<RttThread*>::iterator it = rtt_collection_threads.begin(); it != rtt_collection_threads.end(); it++) {
                switch ((*it)->status) {
                    case WAIT_TO_CREATE: {
                        pthread_attr_t attr;
                        pthread_attr_init(&attr);
                        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                        int ret = pthread_create(&(*it)->id, &attr, doRttCollection,
                                                        (void *)(*it));
                        ALOGI("loop test......%s %ld", (*it)->interface_name.c_str(), (*it)->id);
                        if (ret) {
                            // TODO fail
                            ALOGE("rttManagerThreadLoop rtt thread create failed! %d(%s)", errno, strerror(errno));
                            // delete p_new_thread;
                            // retry on next wake up
                        }
                        else {
                            (*it)->status = USAGE;
                            has_new_thread = true;
                        }
                        break;
                    }
                    case USAGE:
                        // nothing need to do.
                        break;
                    case WAIT_TO_DESTROY:
                        // thread will exit itself.
                        break;
                    default:
                        ALOGE("startRttCollection unknown status: %d", (*it)->status);
                        break;
                }
            }

            rtt_manager_lock.unlock();// unlock

            // do commands.

            //notify
            if (is_expired) {
                wait_time_ms = cleanExpiredResults();
            }
            else {
                wait_time_ms = doNotifyRtt();
            }
            last_notify_time = std::chrono::steady_clock::now();
            rtt_collection_cv.notify_all();
            
            rtt_manager_lock.lock();// lock
            //
        } // release lock
    }
    return NULL;
}

// too long, use dymatic registration instead of static.
// extern "C" JNIEXPORT jint JNICALL Java_com_xiaomi_NetworkBoost_NetworkAccelerateSwitch_NetworkAccelerateSwitchService_startRttCollection(JNIEnv * jniEnv, jobject obj) {
static jint initNetworkAccelerateSwitch(JNIEnv* env, jobject object) {
    int ret = 0;

    mServiceObj = env->NewGlobalRef(object);
    { // acquire lock
        // check to start thread or not.
        if(rtt_manager_thread) return 0;

        std::unique_lock<std::mutex> lock_thread(rtt_manager_lock);
        quit_threads = false;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&rtt_manager_thread, &attr, rttManagerThreadLoop,
                                        NULL);
        if (ret) {
            // TODO fail
            ALOGE("initNetworkAccelerateSwitch manager thread create failed!");
        }
    } // release lock
    return ret;
}

static jint deinitNetworkAccelerateSwitch(JNIEnv* env, jobject object) {
    int ret = 0;

    env->DeleteGlobalRef(mServiceObj);
    mServiceObj = NULL;
    { // acquire lock
        // check to stop thread or not.
        if(!rtt_manager_thread) return 0;
        std::unique_lock<std::mutex> lock_thread(rtt_manager_lock);
        quit_threads = true;
        rtt_manager_thread = 0;
    } // release lock
    return ret;
}

static jint setRttTarget(JNIEnv* env, jobject /*object*/, jstring tcp_address, jint tcp_port, jstring udp_address, jint udp_port, jint target_index) {
    // check index
    if (target_index < 0 || target_index > FALLBACK_TARTGET_INDEX) {
        ALOGE("setRttTarget invalid target index:%d (FALLBACK_TARTGET_INDEX:%d)", target_index, FALLBACK_TARTGET_INDEX);
        return -1;
    }

    int ret = 0;
    std::string str_address;
    bool tcp_valid = true;
    bool udp_valid = true;
    // obtain tcp target
    if (tcp_address != NULL && tcp_port != -1) {
        ret = jstring2string(env, tcp_address, str_address);
        if (ret) {
            ALOGE("setRttTarget can not obtain tcp_address.");
            return -1;
        }
        // memcpy(tcpinfo.server, str_address.c_str(), SERVER_BUFF_SIZE);
        // tcpinfo.port = port;
        // check length of address
        if (strlen(str_address.c_str()) > SERVER_BUFF_SIZE - 1) {
            ALOGE("setRttTarget tcp_address is too long! (%s) len:%d", str_address.c_str(), strlen(str_address.c_str()));
            tcp_valid = false;
        }
        else {
            strcpy(s_tcp_info[TCP_TARTGET_INDEX].server, str_address.c_str());
            s_tcp_info[TCP_TARTGET_INDEX].port = tcp_port;
        }
    }
    else {
        tcp_valid = false;
    }

    // obtain udp target
    if (udp_address != NULL && udp_port != -1) {
        ret = jstring2string(env, udp_address, str_address);
        if (ret) {
            ALOGE("setRttTarget can not obtain udp_address.");
            return -1;
        }
        // check length of address
        if (strlen(str_address.c_str()) > SERVER_BUFF_SIZE - 1) {
            ALOGE("setRttTarget udp_address is too long! (%s) len:%d", str_address.c_str(), strlen(str_address.c_str()));
            udp_valid = false;
        }
        else {
            strcpy(s_tcp_info[UDP_TARTGET_INDEX].server, str_address.c_str());
            s_tcp_info[UDP_TARTGET_INDEX].port = udp_port;
        }
    }
    else {
        udp_valid = false;
    }

    // set target index INVALID_TARTGET_JUDGEMENT
    int tcp_black_cnt = 0;
    int udp_black_cnt = 0;
    { // aquire rtt_collection_lock
        std::unique_lock<std::mutex> lk_for_rtt_thread(rtt_collection_lock);
        for (std::vector<BlackTarget*>::iterator iter = target_black_list.begin(); iter != target_black_list.end(); iter++) {
            if ((*iter)->tcp_info == s_tcp_info[TCP_TARTGET_INDEX]) {
                tcp_black_cnt ++;
            }
            if ((*iter)->tcp_info == s_tcp_info[UDP_TARTGET_INDEX]) {
                udp_black_cnt ++;
            }
        }
    } // release rtt_collection_lock

    if (tcp_black_cnt >= INVALID_TARTGET_JUDGEMENT) {
        ALOGE("setRttTarget tcp target(%s:%d) is invalid because it appear %d time(s) in recent black list"
                , s_tcp_info[TCP_TARTGET_INDEX].server, s_tcp_info[TCP_TARTGET_INDEX].port, tcp_black_cnt);
        tcp_valid = false;
    }
    if (udp_black_cnt >= INVALID_TARTGET_JUDGEMENT) {
        ALOGE("setRttTarget tcp target(%s:%d) is invalid because it appear %d time(s) in recent black list"
                , s_tcp_info[UDP_TARTGET_INDEX].server, s_tcp_info[UDP_TARTGET_INDEX].port, udp_black_cnt);
        udp_valid = false;
    }
    if (!tcp_valid && target_index == TCP_TARTGET_INDEX) {
        if (udp_valid) {
            target_index = UDP_TARTGET_INDEX;
        }
        else {
            target_index = FALLBACK_TARTGET_INDEX;
        }
    }
    else if (!udp_valid && target_index == UDP_TARTGET_INDEX) {
        if (tcp_valid) {
            target_index = TCP_TARTGET_INDEX;
        }
        else {
            target_index = FALLBACK_TARTGET_INDEX;
        }
    }
    tcp_info_index = target_index;
    ALOGI("setRttTarget %d %s:%d", tcp_info_index, s_tcp_info[tcp_info_index].server, s_tcp_info[tcp_info_index].port);
    return 0;
}

static const JNINativeMethod gMethods[] = {
    {"initNetworkAccelerateSwitch", "()I", (void*)initNetworkAccelerateSwitch},
    {"deinitNetworkAccelerateSwitch", "()I", (void*)deinitNetworkAccelerateSwitch},
    {"startRttCollection", "(Ljava/lang/String;)I", (void*)startRttCollection},
    {"stopRttCollection", "()I", (void*)stopRttCollection},
    {"setRttTarget", "(Ljava/lang/String;ILjava/lang/String;II)I", (void*)setRttTarget},
    {"nativeDump", "()Ljava/lang/String;", (void*)nativeDump},
};

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! (var), "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! (var), "Unable to find method " methodName);

#define GET_STATIC_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetStaticMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! (var), "Unable to find static method " methodName);

#define GET_FIELD_ID(var, clazz, fieldName, fieldDescriptor) \
        var = env->GetFieldID(clazz, fieldName, fieldDescriptor); \
        LOG_FATAL_IF(! (var), "Unable to find field " fieldName);


int register_com_xiaomi_NetworkBoost_NetworkAccelerateSwitch(JNIEnv* env) {
    int res = jniRegisterNativeMethods(env, "com/xiaomi/NetworkBoost/NetworkAccelerateSwitch/NetworkAccelerateSwitchService", gMethods,
                                    NELEM(gMethods));
    ALOGI("res. %d", res);
    (void) res;  // Faked use when LOG_NDEBUG.
    LOG_FATAL_IF(res < 0, "Unable to register native methods.");

    jclass clazz;
    // NetworkAccelerateSwitchService
    FIND_CLASS(clazz, "com/xiaomi/NetworkBoost/NetworkAccelerateSwitch/NetworkAccelerateSwitchService");
    mServiceClassInfo.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(clazz));

    GET_METHOD_ID(mServiceClassInfo.notifyRttInfo, clazz, "notifyRttInfo", "([Lcom/xiaomi/NetworkBoost/NetworkAccelerateSwitch/NetworkAccelerateSwitchService$RttResult;)I");
    
    // NetworkAccelerateSwitchService$RttResult
    FIND_CLASS(clazz, "com/xiaomi/NetworkBoost/NetworkAccelerateSwitch/NetworkAccelerateSwitchService$RttResult");
    mRttResultClassInfo.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(clazz));

    GET_METHOD_ID(mRttResultClassInfo.constructor, clazz, "<init>"
            , "(Lcom/xiaomi/NetworkBoost/NetworkAccelerateSwitch/NetworkAccelerateSwitchService;ILjava/lang/String;)V");
    return res;
}