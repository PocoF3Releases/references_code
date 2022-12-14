#ifndef QXWZ_ACCOUNT_H
#define QXWZ_ACCOUNT_H
/**
 * get qx account and store it to the buffer.
 *
 * @param account[out] buffer to store account
 * @param len[in] length of buffer
 * @return 0 succ -1 fail
 */
int32_t getAccount(char* account, const int32_t len);
#endif
