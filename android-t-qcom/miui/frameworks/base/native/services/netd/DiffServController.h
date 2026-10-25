#ifndef _DIFFSERV_CONTROLLER_H_
#define _DIFFSERV_CONTROLLER_H_

class DiffServController {
    public:
        static DiffServController *get();
        int enable();
        int disable();
        int setUidQos(uint32_t protocol, const char* uid, int tos, bool add);
    private:
        DiffServController();
        static DiffServController *sInstance;
        int mEnabled;
};

#endif
