#include "ContextManager.h"
#include "HookManager.h"
#include "EGL/egldefs.h"
namespace android
{
static const int MAX_PACKAGENAME_COUNT = 5;
static const int MAX_PACKAGENAME_LENGTH = 50;
static const int MAX_NAME_LEN = 64;
static const int PROCESS_PATH_LEN = 128;

pthread_key_t ContextManager::sKey = TLS_KEY_HOOK_CONTEXT;
pthread_once_t ContextManager::sOnceKey = PTHREAD_ONCE_INIT;
std::map<const char*, ImplFactory::GAME> ContextManager::gameList =
{
    {"com.tencent.tmgp.sgame", ImplFactory::GAME::SGAME},
    {"com.tencent.tmgp.pubgmhd",ImplFactory::GAME::PUBGMHD}
};

bool ContextManager::hookingEnabled = false;

HookContext ContextManager::nullContext(false,
                                        ImplFactory::getImpl(ImplFactory::GAME::NONE),
                                        ImplFactory::NONE);
HookContext ContextManager::sgameContext(true,
                                         ImplFactory::getImpl(ImplFactory::GAME::SGAME),
                                         ImplFactory::GAME::SGAME);
HookContext ContextManager::pubgmhdContext(true,
                                           ImplFactory::getImpl(ImplFactory::GAME::PUBGMHD),
                                           ImplFactory::GAME::PUBGMHD);

ContextManager::ContextManager(/* args */) {}

ContextManager::~ContextManager() {}

void ContextManager::validateTLSKey() {
    struct TlsKeyInitializer {
        static void create() { pthread_key_create(&sKey, nullptr); }
    };
    pthread_once(&sOnceKey, TlsKeyInitializer::create);
}

void ContextManager::bindAHookContext() {
    validateTLSKey();
    ImplFactory::GAME gameType = getGameType();
    HookContext* context = nullptr;
    switch (gameType) {
        case ImplFactory::GAME::SGAME:
            context = &sgameContext;
            break;

        case ImplFactory::GAME::PUBGMHD:
            context = &pubgmhdContext;
            break;

        default:
            context = &nullContext;
            break;
    }

    context->startMonitor();
    pthread_setspecific(sKey, context);
    if (!ContextManager::hookingEnabled) {
        pthread_setspecific(sKey, &nullContext);
    }
}

HookContext* ContextManager::getCurrentContext() {
    HookContext* ptr = static_cast<HookContext*>(pthread_getspecific(sKey));
    return ptr;
}

HookContext* ContextManager::getNullContext() {
    return &nullContext;
}

void ContextManager::disableHooking() {
    ContextManager::hookingEnabled = false;
}
void ContextManager::enableHooking() {
    ContextManager::hookingEnabled = true;
}

ImplFactory::GAME ContextManager::getGameType() {
    char processName[MAX_NAME_LEN] = {0};
    char buff[PROCESS_PATH_LEN] = {0};
    FILE* file = NULL;

    ImplFactory::GAME gameType = ImplFactory::GAME::NONE;

    if (snprintf(buff, PROCESS_PATH_LEN - 1, "/proc/%d/cmdline", getpid()) < 0) {
        ALOGE("error get process id...");
        return gameType;
    }

    file = fopen(buff, "r");
    if (file == NULL) {
        ALOGE("error open file: %s", buff);
        return gameType;
    }

    if (!fgets(processName, MAX_NAME_LEN, file)) {
        ALOGE("error get process name:%s", buff);
        fclose(file);
        return gameType;
    }

    fclose(file);

    std::map<const char*, ImplFactory::GAME>::iterator iter = gameList.begin();
    for (; iter != gameList.end(); iter++) {
        if (strcmp(processName, iter->first) == 0) {
            gameType = iter->second;
        }
    }
    return gameType;
}
} // namespace android
