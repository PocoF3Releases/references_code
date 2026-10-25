#ifndef _MI_SIGNAL_HANDLER_H_
#define _MI_SIGNAL_HANDLER_H_

#include <setjmp.h>
#include "recovery_ui/ui.h"

extern jmp_buf jb;

void debuggerd_init(RecoveryUI* ui);

#endif /* _MI_SIGNAL_HANDLER_H_ */
