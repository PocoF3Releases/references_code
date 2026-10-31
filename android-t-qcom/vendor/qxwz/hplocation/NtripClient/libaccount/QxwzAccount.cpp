#include <iostream>
#include <com/qxwz/account/1.0/IQxwzAccount.h>
#include "string.h"
#include "QxwzAccount.h"

using namespace std;
using android::hardware::Return;
using android::hardware::Void;
using android::sp;
using com::qxwz::account::V1_0::IQxwzAccount;

int32_t getAccount(char* account, const int32_t len)
{
    if (account == nullptr || len <= 0) {
        return -1;
    }
    sp<IQxwzAccount> qxwzAccountService = IQxwzAccount::getService(false);
    if (qxwzAccountService == nullptr) {
        return -1;
    }
    Return<void> voidReturn = qxwzAccountService->getAccount([&account, len](const auto &res) {strncpy(account, res.c_str(), len);});
    return 0;
}

