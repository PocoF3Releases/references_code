#ifndef _URL_HOOK_CONTROLL_H
#define _URL_HOOK_CONTROLL_H

#include "UrlHookListener.h"
#include "socket_utils.h"

#include <arpa/inet.h>
#include <string>

class UrlHookCommand : public UrlHookListenerCallback {
public:
  UrlHookCommand();
  virtual ~UrlHookCommand();
  int runCommand(SocketClient *client, int argc, char **argv);

  bool processDNSName(const char *name, in_addr_t &addr);

  void sendHostName(const char *name, uid_t uid);

  void sendHookMsg(const void *buf, size_t size);

private:
  SocketClient *mClient;

  struct SubCmd;

  struct SubCmdGroup {
    SubCmd *cmd_head;
    SubCmd *cmd_end;
    SubCmdGroup *next;

    virtual SubCmd *createCommand(char **args, int count, UrlHookCommand* owner) = 0;
    virtual const char *name() = 0;
    void append(SubCmd *cmd);

    void applyAll() {
      SubCmd *cmd = cmd_head;
      while (cmd) {
        cmd->apply();
        cmd = cmd->next;
      }
    }

    void removeAll() {
      SubCmd *cmd = cmd_head;
      while (cmd) {
        SubCmd *tcmd = cmd;
        cmd = cmd->next;
        delete tcmd;
      }
      cmd_head = cmd_end = NULL;
    }

    SubCmdGroup() : cmd_head(NULL), cmd_end(NULL), next(NULL) {}
    virtual ~SubCmdGroup() { removeAll(); }
  };

  template <class TSubCmd> struct SubCmdGroupImpl : public SubCmdGroup {
    const char *name() { return TSubCmd::Name(); }
    SubCmd *createCommand(char **args, int count, UrlHookCommand* owner) {
      return new TSubCmd(args, count, owner);
    }
  };

  struct SubCmd {
    SubCmd *next;
    virtual const char *name() = 0;
    virtual void apply(){};
    virtual void remove(){};
    SubCmd() : next(NULL) {}

    virtual ~SubCmd() { remove(); }
  };

  SubCmdGroup *mSubCmdGroups;
  SubCmdGroup *mIptables;
  SubCmdGroup *mDomains;

  SubCmdGroup *findCmdGroup(const char *name, bool create = false);

  bool processSimpleCmd(const char *name, char **arg, int count);

  static SubCmdGroup *CreateSubCmdGroup(const char *name);

  struct DomainCmd : public SubCmd {
    static const char *Name() { return "domain"; }
    const char *name() { return Name(); }
    char *hostname;
    in_addr_t IPv4;
    UrlHookCommand* owner;

    DomainCmd(char **args, int count, UrlHookCommand* owner);

    void apply();

    void remove() {
      if (hostname)
        free(hostname);
    };
  };

  struct IptablesCmd : public SubCmd {
    static const char *Name() { return "iptables"; }
    const char *name() { return Name(); }
    char **args;
    int count;
    IptablesCmd(char **arg, int count, UrlHookCommand* owner);

    void apply();
    void remove();

    void appendArgs(char **args, int count);
  };

  IptablesCmd *mLastIptables;

  void sendError(const char *cmd);
  void sendGenericOkFail(bool success);

  void sendNfMessage(uid_t uid, const char *prefix, const char *raw, int len);

  bool setupNFLogMonitor(int group, bool use_ulog);
  bool setupSocket(int group, int family, bool confg_as_group);
  bool stopNFLogMonitor();

  int mNFLogFd;

  bool processListener(char **arg, int count);
  bool processSet(char **arg, int count);
  bool processExcute(char **arg, int count);

  UrlHookListener *mListener;

  int mDataServerFd;
  int mUdpPort;
  char *mUnixSocket;
  struct sockaddr_in mUdpServerAddr;
  void initUdpServerAddr();
  void updateUdpPort(int newPort);
  void updateUnixSocket(const char *socket_addr);
  void sendHookMsgByUdp(const void *buf, size_t size);
  void sendHookMsgByLocal(const void *buf, size_t size);
  void sendDNSParsedMsg(const char *dnsname);

  bool mEnableHostEncode;
};

#endif
