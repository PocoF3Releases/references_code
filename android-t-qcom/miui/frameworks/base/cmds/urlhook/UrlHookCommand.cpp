
#include "UrlHookLog.h"
#include <alloca.h>
#include <cutils/log.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "ResponseCode.h"

#include <errno.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <logwrap/logwrap.h>

#include "UrlHookCommand.h"
#include "UrlHookListener.h"
#include "UrlHookMsg.h"

#ifdef URL_HOOK_DEBUG
bool g_url_hook_enable = true;
#else
bool g_url_hook_enable = false;
#endif

static void start_accept_on_connect(int *pclient_fd, const char *unix_name);

void logbuff(const char *prefix, const char *buff, ssize_t count) {
  if (!CAN_DEBUG)
    return;

  char tmp[1024];
  int idx = 0;
  int tidx = 0;
  while (idx < count) {
    if (buff[idx] == '%') {
      tmp[tidx++] = '%';
      tmp[tidx] = '%';
    } else if (buff[idx] >= 32 && buff[idx] <= 127) {
      tmp[tidx] = buff[idx];
    } else {
      static const char hexs[] = "0123456789ABCDEF";
      tmp[tidx++] = '%';
      tmp[tidx++] = hexs[(buff[idx] & 0xf0) >> 4];
      tmp[tidx] = hexs[(buff[idx] & 0xf)];
    }

    tidx++;
    idx++;
    if (tidx >= (int)sizeof(tmp) - 3) {
      tmp[tidx] = 0;
      DEBUG("%s %s", prefix, tmp);
      tidx = 0;
    }
  }
  if (tidx > 0) {
    tmp[tidx] = 0;
    DEBUG("%s %s", prefix, tmp);
  }
}

static UrlHookCommand *_url_hook = NULL;

void UrlHookCommand::SubCmdGroup::append(UrlHookCommand::SubCmd *cmd) {
  if (cmd == NULL)
    return;

  cmd->next = NULL;
  if (cmd_end)
    cmd_end->next = cmd;
  cmd_end = cmd;

  if (cmd_head == NULL) {
    cmd_head = cmd;
  }
}

UrlHookCommand::UrlHookCommand()
    : mClient(NULL), mSubCmdGroups(NULL), mIptables(NULL), mDomains(NULL),
      mLastIptables(NULL), mListener(NULL), mDataServerFd(0), mUdpPort(0),
      mUnixSocket(NULL), mEnableHostEncode(false) {

  ALOGV("UrlHookCommand is create:%p", this);

  initUdpServerAddr();

  if (_url_hook == NULL) {
    _url_hook = this;
  }
}

UrlHookCommand::~UrlHookCommand() {
  SubCmdGroup *group = mSubCmdGroups;

  while (group) {
    SubCmdGroup *tmpg = group;
    group = group->next;
    delete tmpg;
  }
  group = NULL;

  if (mDataServerFd > 0) {
    close(mDataServerFd);
  }

  if (mUnixSocket) {
    free(mUnixSocket);
  }
}

void UrlHookCommand::sendError(const char *cmd) {
  if (mClient) {
    mClient->sendMsg(ResponseCode::CommandSyntaxError, cmd, false);
  }
}

void UrlHookCommand::sendGenericOkFail(bool success) {
  if (!mClient)
    return;
  if (success) {
    mClient->sendMsg(ResponseCode::CommandOkay, "command successed", false);
  } else {
    mClient->sendMsg(ResponseCode::OperationFailed, "command failed", false);
  }
}

int UrlHookCommand::runCommand(SocketClient *client, int argc, char **argv) {
  mClient = client;
#if 0
  for (int i = 0; i < argc; i++) {
    DEBUG("cmd: argv[%d] = %s", i, argv[i]);
  }
#endif

  if (argc < 2) {
    ALOGE("Usage: qadaemon iptables|domain|reset");
    sendError("Usage: qadaemon iptables|domain");
    sendGenericOkFail(false);
    return 0;
  }

  if (mLastIptables != NULL && strcmp(argv[1], "iptables-append") != 0) {
    mLastIptables = NULL;
  }

  if (processSimpleCmd(argv[1], argv + 2, argc - 2)) {
    DEBUG("Processed command:%s", argv[1]);
    sendGenericOkFail(true);
    return 0;
  }

  SubCmdGroup *group = findCmdGroup(argv[1], true);
  if (group == NULL) {
    ALOGE("Cannot find the sub command group :%s", argv[1]);
    sendError("Error command");
    sendGenericOkFail(false);
  } else {
    DEBUG("append %s", group->name());
    SubCmd *cmd = group->createCommand(argv + 2, argc - 2, this);
    group->append(cmd);

    if (strcmp(argv[1], "iptables") == 0) {
      mLastIptables = (IptablesCmd *)cmd;
    }
  }

  sendGenericOkFail(true);
  return 0;
}

UrlHookCommand::SubCmdGroup *UrlHookCommand::findCmdGroup(const char *name,
                                                          bool create) {
  for (SubCmdGroup *group = mSubCmdGroups; group; group = group->next) {
    if (strcmp(group->name(), name) == 0) {
      return group;
    }
  }

  if (!create) {
    return NULL;
  }

  SubCmdGroup *group = CreateSubCmdGroup(name);

  if (group == NULL) {
    return NULL;
  }

  if (mIptables == NULL && strcmp(name, "iptables") == 0) {
    mIptables = group;
  } else if (mDomains == NULL && strcmp(name, "domain") == 0) {
    mDomains = group;
  }

  group->next = mSubCmdGroups;
  mSubCmdGroups = group;
  return group;
}

UrlHookCommand::SubCmdGroup *
UrlHookCommand::CreateSubCmdGroup(const char *name) {
  if (strcmp(name, "iptables") == 0)
    return new SubCmdGroupImpl<IptablesCmd>();
  else if (strcmp(name, "domain") == 0)
    return new SubCmdGroupImpl<DomainCmd>();

  return NULL;
}

bool UrlHookCommand::processSimpleCmd(const char *name, char **arg, int count) {
  if (strcmp(name, "reset") == 0) {
    for (int i = 0; i < count; i++) {
      SubCmdGroup *group = findCmdGroup(arg[i]);
      if (group) {
        DEBUG("reset group %s", group->name());
        group->removeAll();
      }
    }
  } else if (strcmp(name, "apply") == 0) {
    for (int i = 0; i < count; i++) {
      DEBUG("apply %s", arg[i]);
      if (strcmp(arg[i], "all") == 0) {
        SubCmdGroup *group = mSubCmdGroups;
        while (group) {
          group->applyAll();
          group = group->next;
        }
        break;
      } else {
        SubCmdGroup *group = findCmdGroup(arg[i]);
        if (group) {
          group->applyAll();
        }
      }
    }
    DEBUG("apply end");
  } else if (strcmp(name, "listener") == 0) {
    return processListener(arg, count);
  } else if (strcmp(name, "set") == 0) {
    return processSet(arg, count);
  } else if (strcmp(name, "iptables-append") == 0) {
    if (mLastIptables == NULL) {
      ALOGE("iptables-append must be called after the iptables command");
      return false;
    }
    mLastIptables->appendArgs(arg, count);
  } else if (strcmp(name, "excute") == 0) {
    return processExcute(arg, count);
  } else {
    return false;
  }

  return true;
}

static inline bool to_boolean(const char *str) {
  return (strcasecmp(str, "true") == 0) || (strcasecmp(str, "yes") == 0) ||
         (strcasecmp(str, "y") == 0) || (strcmp(str, "1") == 0);
}

bool UrlHookCommand::processSet(char **arg, int count) {
  if (count < 2) {
    ALOGE("set command must has key_name value");
  }

  if (strcmp(arg[0], "debug") == 0) {
    bool enable = to_boolean(arg[1]);
    ALOGI("enable debug");
    ENABLE_DEBUG(enable);
  } else if (strcmp(arg[0], "udp-port") == 0) {
    updateUdpPort(atoi(arg[1]));
  } else if (strcmp(arg[0], "unix-domain") == 0) {
    updateUnixSocket(arg[1]);
  } else if (strcmp(arg[0], "enable-host-encode") == 0) {
    mEnableHostEncode = to_boolean(arg[1]);
  } else {
    ALOGE("Unkown set value name:%s", arg[0]);
    return false;
  }
  return true;
}

bool UrlHookCommand::processDNSName(const char *name, in_addr_t &addr) {
   ALOGI("processDNSName:%s", name);
  if (!mDomains) {
    ALOGI("processDNSName line:%d", __LINE__);
    return false;
  }
  ALOGI("processDNSName line:%d", __LINE__);
  SubCmd *cmd = mDomains->cmd_head;
  while (cmd) {
    DomainCmd *dcmd = (DomainCmd *)cmd;
    ALOGI("processDNSName hostname:%s", dcmd->hostname);
    if (strcmp(dcmd->hostname, name) == 0) {
      addr = dcmd->IPv4;
      sendDNSParsedMsg(dcmd->hostname);
      return true;
    }

    cmd = cmd->next;
  }

  return false;
}

void UrlHookCommand::sendDNSParsedMsg(const char *domain) {
  unsigned char szbuf[512];
  MessageStream ms(szbuf);

  ms.put((char)0);
  ms.put((char)TYPE_NOTIFY);
  ms.put(KEY_HOSTNAME, domain, strlen(domain));

  sendHookMsg(szbuf, ms.size());
  logbuff("dns parsed:", (const char *)szbuf, ms.size());
}

void UrlHookCommand::sendHostName(const char *name, uid_t uid) {
  if (!mEnableHostEncode || name == NULL)
    return;

  unsigned char szbuf[1024];
  MessageStream ms(szbuf);

  ms.put((char)0); // special code
  ms.put((char)TYPE_DOMAIN);
  ms.put(KEY_UID, uid);

  size_t len = strlen(name);

  if (len >= sizeof(szbuf) - ms.size() - 5) { // key size (1) + length size(4)
    DEBUG("Host name is too large:%d", (int)len);
    return;
  }
  ms.put(KEY_HOSTNAME, name, (int)len);

  sendHookMsg(szbuf, ms.size());
  logbuff("dns:", (const char *)szbuf, ms.size());
}

UrlHookCommand::DomainCmd::DomainCmd(char **args, int count, UrlHookCommand* owner) {
  this->owner = owner;
  hostname = NULL;
  if (count <= 1) {
    // error
    return;
  }
  hostname = strdup(args[0]);
  // parse iptable
  IPv4 = inet_addr(args[1]);
  DEBUG("create domain:%s %s", args[0], args[1]);
}

void UrlHookCommand::DomainCmd::apply() {
  if (hostname && owner) {
    //owner->sendDNSParsedMsg(hostname);
  }
}

///////////////////////////////////////////////////////

#if PLATFORM_SDK_VERSION >= 27
const char *const IPTABLES_PATH = "/system/bin/iptables";
#endif

static const char *iptables_pre_args[] = {IPTABLES_PATH,
#if PLATFORM_SDK_VERSION >= 21
                                          "-w"
#endif
};
#define IPTABLES_PRE_SIZE                                                      \
  (sizeof(iptables_pre_args) / sizeof(iptables_pre_args[0]))

static void init_iptables_pre_args(char **args) {
  for (int i = 0; i < (int)IPTABLES_PRE_SIZE; i++) {
    args[i] = (char *)iptables_pre_args[i];
  }
}

UrlHookCommand::IptablesCmd::IptablesCmd(char **args, int count, UrlHookCommand*) {
  this->args = NULL;
  this->count = 0;
  if (count <= 0)
    return;

  appendArgs(args, count);
}

void UrlHookCommand::IptablesCmd::appendArgs(char **args, int count) {
  if (count <= 0 || args == NULL) {
    return;
  }

  int new_count = 0;
  char **new_args = NULL;
  int start = 0;

  // remove the "" args
  char **temp_args = NULL;
  int temp_count = 0;
  temp_args = (char **)alloca(sizeof(char *) * count);
  for (int i = 0; i < count; i++) {
    if (args[i] && strcmp(args[i], "") != 0) {
      temp_args[temp_count] = args[i];
      temp_count++;
    }
  }

  if (temp_count <= 0) {
    return;
  }

  if (this->args == NULL || this->count <= 0) {
    new_count = temp_count + IPTABLES_PRE_SIZE + 1;
    start = IPTABLES_PRE_SIZE;
    new_args = (char **)malloc(new_count * sizeof(char *));
    init_iptables_pre_args(new_args);
  } else {
    new_count = this->count + temp_count;
    start = this->count - 1;
    new_args = (char **)realloc(this->args, new_count * sizeof(char *));
  }

  for (int i = 0; i < temp_count; i++) {
    new_args[i + start] = strdup(temp_args[i]);
  }

  new_args[new_count - 1] = NULL;

  this->args = new_args;
  this->count = new_count;
}

#if 1
static int execIptablesCommand(int argc, const char *argv[], bool silent) {
  int res;
  int status;

  res = logwrap_fork_execvp(argc, (char **)argv, &status, false, LOG_ALOG, false, nullptr);
  if (res || !WIFEXITED(status) || WEXITSTATUS(status)) {
    if (!silent) {
      ALOGE("execute iptables faield: ");
    }
    if (res)
      return res;
    if (!WIFEXITED(status))
      return ECHILD;
  }
  return WEXITSTATUS(status);
}
#else
static void execIptablesCommand(int argc, const char *argv[], bool silent) {
  pid_t  pid = fork();
  if (pid == 0) {
    execvp();
  }
}
#endif


void UrlHookCommand::IptablesCmd::apply() {
  if (count > 0) {
    std::string commands;
    for (int i = 0; i < count - 1; i++) {
      commands += args[i];
      commands += " ";
    }

    if (execIptablesCommand(count, (const char **)args, false) == 0) {
      DEBUG("apply exec %s ok!", commands.c_str());
    } else {
      ALOGE("apply exec %s failed!", commands.c_str());
    }
  }
}

void UrlHookCommand::IptablesCmd::remove() {
  if (args == NULL || count <= 0)
    return;
  // delete the iptables
  char **args = this->args;
  int count = this->count;
  this->args = NULL;
  this->count = 0;
  for (int i = IPTABLES_PRE_SIZE; i < count; i++) {
    if (args[i])
      free(args[i]);
  }
  free(args);
}

bool UrlHookCommand::processExcute(char **arg, int count) {
  if (count < 2) {
    ALOGE("set command must has key_name value");
  }

  std::string commands;
  for (int i = 0; i < count; i++) {
    commands += arg[i];
    commands += " ";
  }

  if (execIptablesCommand(count, (const char **)arg, false) == 0) {
    DEBUG("processExcute exec %s ok!", commands.c_str());
  } else {
    ALOGE("processExcute exec %s failed!", commands.c_str());
  }
  return true;
}

bool UrlHookCommand::processListener(char **args, int count) {
  if (count <= 0)
    return false;

  if (strcmp(args[0], "start") == 0) {
    if (mListener != NULL) {
      DEBUG("Listener is started!");
      return true;
    }

    UrlHookListener *listener =
        UrlHookListener::createFromArgs(args + 1, count - 1, this);
    if (listener) {
      if (listener->start()) {
        mListener = listener;
        return true;
      }
      ALOGE("Listener start faield!");
      delete listener;
    }
    return false;
  } else if (strcmp(args[0], "stop") == 0) {
    if (mListener) {
      mListener->stop();
      delete mListener;
      mListener = NULL;
    }
  } else {
    ALOGE("Unknwon listener command");
    return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////////////
// udp send

void UrlHookCommand::initUdpServerAddr() {
  memset(&mUdpServerAddr, 0, sizeof(mUdpServerAddr));
  mUdpServerAddr.sin_family = AF_INET;
  mUdpServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  mUdpServerAddr.sin_port = htons(mUdpPort);
  mDataServerFd = 0;
}

void UrlHookCommand::updateUdpPort(int newPort) {
  if (mUdpPort != newPort) {
    mUdpPort = newPort;

    if (mUnixSocket == NULL && mDataServerFd > 0) {
      volatile int fd = mDataServerFd;
      mDataServerFd = 0;
      close(fd);
    }
    initUdpServerAddr();
  }
}

void UrlHookCommand::updateUnixSocket(const char *socket_addr) {
  if (mUnixSocket && strcmp(mUnixSocket, socket_addr) == 0) {
    return;
  }

  if (mUnixSocket) {
    free(mUnixSocket);
  }

  mUnixSocket = strdup(socket_addr);

  if (mDataServerFd > 0) {
    volatile int fd = mDataServerFd;
    mDataServerFd = 0;
    close(fd);
  }

  start_accept_on_connect(&mDataServerFd, mUnixSocket);
}

void UrlHookCommand::sendHookMsg(const void *buf, size_t size) {
  bool useUnixDomain = (mUnixSocket != NULL);

  if (useUnixDomain) {
    sendHookMsgByLocal(buf, size);
  } else {
    sendHookMsgByUdp(buf, size);
  }
}

void UrlHookCommand::sendHookMsgByUdp(const void *buf, size_t size) {

  if (mDataServerFd <= 0) {
    if (mUdpPort > 0) {
      mDataServerFd = socket(AF_INET, SOCK_DGRAM, 0);
      DEBUG("UDPServerConnector create fd=%d, port=%d", mDataServerFd,
            mUdpPort);
    }
  }

  if (mUdpPort == 0) {
    return;
  }

  struct sockaddr *addr = (struct sockaddr *)&mUdpServerAddr;
  socklen_t len = sizeof(mUdpServerAddr);

  if (mDataServerFd < 0) {
    ALOGE("UDPServerConnector failed: %s", strerror(errno));
    mDataServerFd = 0; // try connect next time
    return;
  }

  if (sendto(mDataServerFd, buf, size, 0, addr, len) < 0) {
    ALOGE("UDPServerConnector send failed %s", strerror(errno));
    return;
  }
  DEBUG("UDPServerConnector send %zu bytes", size);
}

void UrlHookCommand::sendHookMsgByLocal(const void *buf, size_t size) {

  if (mDataServerFd == 0) {
    ALOGE("The wmservice have not connected!");
    return;
  }

  if (send(mDataServerFd, buf, size, 0) != (ssize_t)size) {
    ALOGE("Send Hook Message Failed! cannot send all %d bytes data:%s",
          (int)size, strerror(errno));
  }
  ALOGE("Send Hook Message OK %d bytes data mDataServerFd:%d", (int)size, mDataServerFd);
}

/////////////////////////////////////////////////////////////////////////////
struct local_accept_info {
  int *pclient_fd;
  socklen_t addr_len;
  struct sockaddr_un addr;
};

static void *_wait_accept(void *param) {
  local_accept_info *pinfo = (local_accept_info *)param;
  struct sockaddr_un caddr;
  socklen_t clen = sizeof(caddr);

  int sfd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (bind(sfd, (struct sockaddr *)&(pinfo->addr), pinfo->addr_len) != 0) {
    ALOGE("cannot create server:%s", pinfo->addr.sun_path + 1);
    goto END;
  }

  listen(sfd, 1);

  *(pinfo->pclient_fd) = accept(sfd, (struct sockaddr *)&caddr, &clen);
  DEBUG("Send message to wms sockect accept OK fd:%d", *(pinfo->pclient_fd));

END:
  close(sfd);
  delete pinfo;
  return NULL;
}

static void start_accept_on_connect(int *pclient_fd, const char *unix_name) {
  local_accept_info *pinfo = new local_accept_info;
  pinfo->pclient_fd = pclient_fd;
  InitUnixDomain(unix_name, &(pinfo->addr), &(pinfo->addr_len));

  pthread_t th;
  pthread_create(&th, NULL, _wait_accept, pinfo);
  pthread_detach(th);
}
