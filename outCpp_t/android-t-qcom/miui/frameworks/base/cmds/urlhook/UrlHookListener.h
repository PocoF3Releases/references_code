#ifndef URL_HOOK_LISTENER_H
#define URL_HOOK_LISTENER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class UrlHookListenerCallback {
public:
  virtual ~UrlHookListenerCallback() {}
  virtual void sendHookMsg(const void *buff, size_t count) = 0;
};

class UrlHookListener {
  UrlHookListenerCallback *mCallback;

public:
  UrlHookListener() {}
  virtual ~UrlHookListener() {}

  static UrlHookListener *createFromArgs(char **args, int count,
                                         UrlHookListenerCallback *cb);

  virtual bool start() = 0;

  virtual bool stop() = 0;

protected:
  void sendHookMsg(const void *buff, size_t count) {
    if (mCallback)
      mCallback->sendHookMsg(buff, count);
  }
};

struct ArgCfg {
  char **args;
  int count;

  ArgCfg(char **args, int count) : args(args), count(count) {}

  int getInt(const char *option) {
    char **a = getOption(option);
    if (a)
      return atoi(a[1]);
    return 0;
  }

  int isSet(const char *option) { return getOption(option, false) != NULL; }

  const char *getString(const char *option) {
    char **a = getOption(option);
    return a ? a[1] : NULL;
  }

private:
  char **getOption(const char *option, bool has_value = true) {
    for (int i = 0; i < count - (has_value ? 1 : 0); i++) {
      if (strcmp(args[i], option) == 0) {
        return args + i;
      }
    }
    return NULL;
  }
};

uint32_t find_uid_by_ip_packet(void *pkt, int len);

template <typename T = char, size_t BUF_SIZE = 1024> class Buffer {
public:
  Buffer() : buf_(new T[BUF_SIZE]) { memset(buf_, 0, BUF_SIZE * sizeof(T)); }

  ~Buffer() { delete[] buf_; }

  T *Data() { return buf_; }

  static size_t Size() { return BUF_SIZE * sizeof(T); }

private:
  T *buf_;
};

#endif
