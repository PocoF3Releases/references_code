#ifndef PROC_REPORTER_DELEGATE_H
#define PROC_REPORTER_DELEGATE_H 

#include <utils/RefBase.h>

namespace android {

class ProcReporterDelegate : public RefBase {

public:
    static ProcReporterDelegate* getInstance() {
        if (instance == NULL)
            instance = new ProcReporterDelegate();

        return instance;
    }

private:
    ProcReporterDelegate();
    virtual ~ProcReporterDelegate();

    static ProcReporterDelegate* instance;

    void* pr_impl;
    void* pr_start;
    void* pr_stop;
};

}
#endif

