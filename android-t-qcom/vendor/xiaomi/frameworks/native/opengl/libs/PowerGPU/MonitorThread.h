#ifndef ANDROID_MONITOR_THREAD_H
#define ANDROID_MONITOR_THREAD_H
#include <fcntl.h>
#include <stddef.h>
#include <sys/poll.h>

#include "GameImpl/ImplFactory.h"

#define MAX_GAME_PARAMETER_SIZE (100)

/*
game code map

For com.tencent.tmgp.sgame:
1) game code             :  1
2）Game enter foreground ：”1#1“
3）Game enter background ：”1#2“
4）Game in lobby         ： ”1#4“
5）Game in battling      ：”1#5“
6）Game in replaying     ：”1#6“
*/


namespace android {

using namespace std;

#define NAME                   "name"
#define STATUS                 "status"
#define PICTURE                "picture"
#define RESOLUTION             "resolution"
#define ENABLE                 "enable"
#define FBOFACTOR              "FB0Factor"
#define GAMEINTERNALFBOFACTOR  "gameInternalFBOFactor"
#define GAMEVERSION             "gameVersion"
#define INTERNALENABLE          "internalEnable"

#define SGAME_CORE  (1)
#define SGAME_STATUS_REPLAYING  (8)
#define SGAME_STATUS_BATTLEING  (7)


extern map<std::string,int>  gameInfo;

class MonitorThread {
   public:
    MonitorThread();
    MonitorThread(ImplFactory::GAME type);
    void start();
    ~MonitorThread();

   private:
    pthread_t eventThread;
    bool isStarted;
    ImplFactory::GAME gameType;
    static void* threadProc(void* context);
    void* eventProcess();

    static constexpr const char* kGameParameterPath =
        "/sys/class/gpu_plaid/plaid/game_parameters";
    pollfd mPollFd;

   protected:
    bool parseprocessData(const char* data, int size);
    void initGameInfo();
};
}  // namespace android

#endif