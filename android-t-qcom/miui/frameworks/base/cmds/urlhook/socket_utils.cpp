
#include "socket_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "UrlHookLog.h"

void InitUnixDomain(const char *path, struct sockaddr_un *pun,
                    socklen_t *plen) {
  *plen = 0;
  if (path == NULL) {
    return;
  }

  *plen = offsetof(struct sockaddr_un, sun_path);

  memset(pun, 0, sizeof(struct sockaddr_un));
  pun->sun_family = AF_UNIX;
  /*if (path[0] == '/') {
    strncpy(pun->sun_path, path, sizeof(pun->sun_path) - 1);
    *plen = strlen(pun->sun_path);
  } else*/
  { // abstract unix name:  sun_path[0] == '\0',sun_path + 1 == path
    strncpy(pun->sun_path + 1, path, sizeof(pun->sun_path) - 2);
    *plen += strlen(pun->sun_path + 1) + 1;
  }
}

SocketClient::~SocketClient() {
  if (mFd > 0) {
    close(mFd);
  }
}

int SocketClient::sendMsg(int code, const char* msg, bool addErrNo) {
  char* buf;
  int ret = 0;

  if (addErrNo) {
    ret = asprintf(&buf, "%d %d %s (%s)", code, mCmdNum, msg, strerror(errno));
  } else {
    ret = asprintf(&buf, "%d %d %s", code, mCmdNum, msg);
  }

  if (ret != -1) {
    ret = sendData(buf, strlen(buf) + 1);
    free(buf);
  }

  return ret;
}

int SocketClient::sendData(const char* msg, int len) {
  int cur = 0;

  DEBUG(LOG_TAG " send msg: %d: %s", mFd, msg);

  while (cur < len) {
    int r = write(mFd, msg + cur, len - cur);
    if (r <= 0) {
      break;
    }
    cur += r;
  }

  return cur == len ? cur : -1;
}

void SocketClient::setClosing(bool value) {
  mClosing = value;
}

bool SocketClient::getClosing() {
  return mClosing;
}
