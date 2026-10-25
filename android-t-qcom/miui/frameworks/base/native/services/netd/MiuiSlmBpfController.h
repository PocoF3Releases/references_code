#ifndef MIUI_SLM_BPF_CONTROLLER_H_
#define MIUI_SLM_BPF_CONTROLLER_H_

class MiuiSlmBpfController   {
    public:
        static MiuiSlmBpfController *get();
        unsigned int getMiuiSlmVoipUdpPort(int uid);
        unsigned int getMiuiSlmVoipUdpAddress(int uid);
        bool setMiuiSlmBpfUid(int uid);

    private:
        MiuiSlmBpfController();
        int writeMiuiSlmBpfUid(int uid);
        unsigned int getMiuiSlmVoipUdpAddressAndPort(int uid, int opt);

        static MiuiSlmBpfController *sInstance;
};

#endif