#ifndef _SOCKET_CLINET_H_
#define _SOCKET_CLINET_H_

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

class SocketClient {
public:
  SocketClient(int fd) : mClosing(false), mFd(fd), mCmdNum(0) {}
  ~SocketClient();

  void setCmdNum(int cmdNum) { mCmdNum = cmdNum; }

  int sendMsg(int code, const char *buffer, bool addErrNo);

  int fd() const { return mFd; }
  void setClosing(bool value);
  bool getClosing();

 private:
  int sendData(const char* msg, int len);

  bool mClosing;
  int mFd;
  int mCmdNum;
};

void InitUnixDomain(const char *path, struct sockaddr_un *pun, socklen_t *plen);

#endif // _SOCKET_CLINET_H_
