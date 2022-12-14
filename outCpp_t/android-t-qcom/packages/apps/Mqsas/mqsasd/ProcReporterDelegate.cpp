#define LOG_TAG "mqsasd pr"

#include <dlfcn.h>
#include "ProcReporterDelegate.h"

#define PR_LIB_NAME  "libprocreporter.so"
#define PR_FUN_START "procreporter_start"
#define PR_FUN_STOP  "procreporter_stop"

typedef void (*PR_DefaultFunPtr)();

namespace android {

ProcReporterDelegate* ProcReporterDelegate::instance = nullptr;

ProcReporterDelegate::ProcReporterDelegate() {
    this->pr_impl = dlopen(PR_LIB_NAME, RTLD_NOW);

    if (this->pr_impl) {
        this->pr_start = dlsym(pr_impl, PR_FUN_START);
        this->pr_stop  = dlsym(pr_impl, PR_FUN_STOP);

        if (this->pr_start)
            ((PR_DefaultFunPtr)this->pr_start)();
    }
}

ProcReporterDelegate::~ProcReporterDelegate() {
    if (this->pr_impl) {
        if (this->pr_stop)
            ((PR_DefaultFunPtr)this->pr_stop)();

        dlclose(this->pr_impl);
    }
}

}

