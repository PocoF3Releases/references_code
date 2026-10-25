#ifndef _NETD_CONSTANTS_EXTEND_H
#define _NETD_CONSTANTS_EXTEND_H

#include "NetdConstants.h"

#include "IptablesRestoreController.h"

int execIptablesRestoreExtend(IptablesTarget target, const std::string& commands);
int enableIptablesRestore(bool enabled);

static int (*execIptablesRestoreFunction)(IptablesTarget target, const std::string& commands) = execIptablesRestore;
#endif  //_NETD_CONSTANTS_EXTEND_H
