#include "MonitorThread.h"

#include <poll.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/syscall.h>

#include "HookManager.h"

#undef LOG_TAG
#define LOG_TAG "EGL_MONITOR"

namespace android {

map<std::string,int>  gameInfo;

MonitorThread::MonitorThread(){};

MonitorThread::MonitorThread(ImplFactory::GAME type)
    : isStarted(false), gameType(type) {}

void MonitorThread::start() {
    if (gameType == ImplFactory::GAME::NONE) return;
    if (isStarted) return;

    if (pthread_create(&eventThread, NULL, &android::MonitorThread::threadProc,
                       this) < 0) {
        ALOGE("Create MonitorThread ERROR.");
        return;
    }

    initGameInfo();

    mPollFd.fd = open(kGameParameterPath, O_RDONLY);
    if (mPollFd.fd < 0)
        ALOGE(" open %s failed", kGameParameterPath);

    mPollFd.events = POLLPRI | POLLERR;

    isStarted = true;
}

void* MonitorThread::threadProc(void* context) {
    if (context) {
        return reinterpret_cast<MonitorThread*>(context)->eventProcess();
    }

    return nullptr;
}

void* MonitorThread::eventProcess() {

    int ret;
    int readBytes;
    char temp_buf[MAX_GAME_PARAMETER_SIZE] = {0};

    if (mPollFd.fd < 0) 
        return nullptr;

    ALOGD("start eventProcess");

    while (1) {
        ret = poll(&mPollFd, 1, -1);
        if (ret <= 0) {
            ALOGE(" poll failed err= %s", strerror(errno));
            continue;
        }

        if (mPollFd.revents & POLLPRI) {
            std::memset(temp_buf, 0x00, sizeof(temp_buf));

            readBytes = pread(mPollFd.fd, temp_buf, MAX_GAME_PARAMETER_SIZE, 0);
            if (readBytes < 0) {
                ALOGE("pread failed");
                continue;

            } else {
                ALOGD("read(%d) %s ", readBytes, temp_buf);
                parseprocessData(temp_buf,readBytes);
            }
        } else {
            ALOGI("loop again");
        }
    }
    ALOGD("end eventProcess");
    return nullptr;
}

void MonitorThread::initGameInfo() {
    gameInfo[NAME] = -1;
    gameInfo[STATUS] = -1;
    gameInfo[PICTURE] = -1;
    gameInfo[RESOLUTION] = -1;
    gameInfo[ENABLE] = -1;
    gameInfo[FBOFACTOR] = -1;
    gameInfo[GAMEINTERNALFBOFACTOR] = -1;
    gameInfo[GAMEVERSION] = -1;
    gameInfo[INTERNALENABLE] = 0;
}


bool MonitorThread::parseprocessData(const char* data, int size) {
    // name1:value1#/name2:value2#/name3:value3#....#

    string  dataString= string(data);
    string keyString = "#";
    int position, cutStart, loopTimes;

    position = 0;  // research form the first char
    cutStart = 0;
    loopTimes = 0;

    while ((position = dataString.find(keyString, position)) !=
           std::string::npos) {

        string unitString = dataString.substr(cutStart, position - cutStart);
        
        int colonPos = unitString.find(":", 0);
        string nameString = unitString.substr(0, colonPos);
        int value = stoi(unitString.substr(colonPos + 1, unitString.size()));

        ALOGV("unitString: %s position=%d colonPos=%d %s", unitString.c_str(), position,colonPos,nameString.c_str());

        map<string,int>::iterator iter;
        iter = gameInfo.find(nameString);
        if(iter != gameInfo.end()){
            gameInfo[nameString] = value;
            ALOGD("name:%s,value:%d ", nameString.c_str(), value);

        } else {
            ALOGE("unknown values:name:%s,value:%d", nameString.c_str(), value);
        }
        cutStart = ++position;
    }
    //ALOGD("%d %d  ",gameInfo[STATUS],gameInfo[ENABLE]);

   bool enable = (gameInfo[STATUS] == SGAME_STATUS_REPLAYING) || (gameInfo[STATUS] == SGAME_STATUS_BATTLEING);
   enable &= (gameInfo[ENABLE] == 1);
   
   if(enable) {
       gameInfo[INTERNALENABLE] = 1;
   } else {
       gameInfo[INTERNALENABLE] = 0; 
   }


    return true;
}

MonitorThread::~MonitorThread() { 
    close(mPollFd.fd);
    initGameInfo();
    ALOGD("end MonitorThread");
}

}  // namespace android
